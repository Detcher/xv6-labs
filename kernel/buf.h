struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;      // entry.key
  struct sleeplock lock;
  uint refcnt;
  struct buf *prev;
  struct buf *next;  // entry.next
  uchar data[BSIZE]; // entry.value
};

