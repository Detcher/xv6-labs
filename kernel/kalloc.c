// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct spinlock pg_rc_lock;

#define PAGE_INDEX(p) (((p) - KERNBASE) / PGSIZE)
#define PAGE_NUM PAGE_INDEX(PHYSTOP)
int pg_rc[PAGE_NUM]; // From KERNBASE to PHYSTOP
                     // Well, i know it's overestimated, though:(
#define PAGE_RC(p) pg_rc[PAGE_INDEX((uint64)p)]

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&pg_rc_lock, "page ref-count");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  // D: For debugging
  if(((uint64)pa % PGSIZE) != 0) {
    panic("kfree: not page-aligned");
  }
  if((char*)pa < end) {
    panic("kfree: pa too low");
  }
  if((uint64)pa >= PHYSTOP) {
    panic("kfree: pa too high");
  }

  acquire(&pg_rc_lock);
  decrease_pg_rc((void *)pa);
  if( PAGE_RC(pa) <= 0 ) {
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
  release(&pg_rc_lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk
    PAGE_RC(r) = 1;
  }
  return (void*)r;
}

void
increase_pg_rc(void *pa)
{
  acquire(&pg_rc_lock);
  PAGE_RC(pa)++;
  release(&pg_rc_lock);
}

void
decrease_pg_rc(void *pa)
{
  PAGE_RC(pa)--;
}

void *
new_page_or_self(void *pa)
{
  acquire(&pg_rc_lock);
  if( PAGE_RC(pa) <= 1 ) { // D: is "<" necessary?
    release(&pg_rc_lock);
    return pa;
  }

  uint64 cow_page = (uint64)kalloc();
  if( cow_page == 0 ) {
    /*
    ** 'panic()' here maybe not appropriate. 
    ** Leave the option to the user/code using 'new_page_or_self'
    */
    // panic("new_page_or_self: kalloc went wrong, no sufficient memory");
    release(&pg_rc_lock);
    return 0;
  }
  memmove( (void *)cow_page, pa, PGSIZE );

  // decrease_pg_rc(pa);
  release(&pg_rc_lock);
  return (void *)cow_page;
}