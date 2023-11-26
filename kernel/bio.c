// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13
#define HASH(index) (index % NBUCKET)

struct bucket {
  struct buf head;
  struct spinlock bucket_lck;
};

struct {
  struct buf buf[NBUF]; // D: buf pool!
  struct bucket bcache_tbl[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;
  char lck_name[NBUCKET];

  for( int i = 0; i < NBUCKET; ++i ) {
    snprintf( lck_name, sizeof(lck_name), "bcache_%d", i );
    initlock( &bcache.bcache_tbl[i].bucket_lck, lck_name );

    bcache.bcache_tbl[i].head.next = bcache.bcache_tbl[i].head.prev = &bcache.bcache_tbl[i].head;
  }

  for( b = bcache.buf; b < bcache.buf + NBUF; b++ ) {
    initsleeplock(&b->lock, "buffer");
    b->next = b->prev = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  acquire(&bcache.bcache_tbl[HASH(blockno)].bucket_lck);
  
  struct buf *b, *empty = 0; 
  // Is the block already cached?
  for( b = bcache.bcache_tbl[HASH(blockno)].head.next; b != &bcache.bcache_tbl[HASH(blockno)].head; b = b->next ) {
    if( b->refcnt == 0 ) // Q: why don't add "empty == 0" in the condition?
      empty = b;         // A: to find the last empty slot passingly.
                         // maybe better find it in reverse, but... PASSINGly:)
    if( b->dev == dev && b->blockno == blockno ) {
      b->refcnt++;
      release(&bcache.bcache_tbl[HASH(blockno)].bucket_lck);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // D: corresponding bucket first,
  if( empty != 0 )
    goto found;
  // otherwise stealing blanky buf(refcnt==0) from pool.
  // must be elsewhere other than the bucket above.
  // TWO states of different blanky buf:
  //    - some buckets
  //    - nowhere but pool
  for( int i = 0; i < NBUF; i++ ) {
    if( HASH(bcache.buf[i].blockno) == HASH(blockno) )
      continue;

    acquire(&bcache.bcache_tbl[HASH(bcache.buf[i].blockno)].bucket_lck);
    if( bcache.buf[i].prev == &bcache.buf[i] && bcache.buf[i].next == &bcache.buf[i] ) {
      // pool; And its refcnt must be 0.
      empty = &bcache.buf[i];
      empty->next = b->next;
      empty->prev = b;
      b->next->prev = empty;
      b->next = empty;

      release(&bcache.bcache_tbl[HASH(bcache.buf[i].blockno)].bucket_lck);
      goto found;
    } else {
      // buckets; And must acquire bucket lock first before accessing refcnt field.
      if( bcache.buf[i].refcnt == 0 ) {
        int backup = bcache.buf[i].blockno;
        bcache.buf[i].dev = dev;
        bcache.buf[i].blockno = blockno;
        bcache.buf[i].valid = 0;
        bcache.buf[i].refcnt = 1;
        bcache.buf[i].next->prev = bcache.buf[i].prev;
        bcache.buf[i].prev->next = bcache.buf[i].next;
        bcache.buf[i].next = b->next;
        bcache.buf[i].prev = b;
        b->next->prev = &bcache.buf[i];
        b->next = &bcache.buf[i];

        release(&bcache.bcache_tbl[HASH(backup)].bucket_lck);
        release(&bcache.bcache_tbl[HASH(blockno)].bucket_lck);
        acquiresleep(&bcache.buf[i].lock);
        return &bcache.buf[i]; 
      } 
      release(&bcache.bcache_tbl[HASH(bcache.buf[i].blockno)].bucket_lck);
    }
  }
  panic("bget: no buffers");

found:
  empty->dev = dev;
  empty->blockno = blockno;
  empty->valid = 0;
  empty->refcnt = 1;

  release(&bcache.bcache_tbl[HASH(blockno)].bucket_lck);
  acquiresleep(&empty->lock);
  return empty;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int index = HASH(b->blockno);
  acquire(&bcache.bcache_tbl[index].bucket_lck);
  b->refcnt--;
  if( b->refcnt == 0 ) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.bcache_tbl[index].head.next;
    b->prev = &bcache.bcache_tbl[index].head;
    bcache.bcache_tbl[index].head.next->prev = b;
    bcache.bcache_tbl[index].head.next = b;
  }
  release(&bcache.bcache_tbl[index].bucket_lck);
}

void
bpin(struct buf *b) {
  int index = HASH(b->blockno);
  acquire(&bcache.bcache_tbl[index].bucket_lck);
  b->refcnt++;
  release(&bcache.bcache_tbl[index].bucket_lck);
}

void
bunpin(struct buf *b) {
  int index = HASH(b->blockno);
  acquire(&bcache.bcache_tbl[index].bucket_lck);
  b->refcnt--;
  release(&bcache.bcache_tbl[index].bucket_lck);
}


