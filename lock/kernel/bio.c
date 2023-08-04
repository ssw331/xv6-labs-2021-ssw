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

#define NBUCKETS 13

struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKETS];
} bcache;

int
hash(uint no) {
    return no % NBUCKETS;
}

void
binit(void)
{
  struct buf *b;

  for(int i = 0; i < NBUCKETS; i++)
    initlock(&bcache.lock[i], "bcache");

  // Create linked list of buffers
  for(int i = 0; i < NBUCKETS; i++) {
      bcache.head[i].prev = &bcache.head[i];
      bcache.head[i].next = &bcache.head[i];
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++) {  // 给cache的零号位添加缓冲区内容
      b->next = bcache.head[0].next;
      b->prev = &bcache.head[0];
      initsleeplock(&b->lock, "buffer");
      bcache.head[0].next->prev = b;
      bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int idx = hash(blockno);

  acquire(&bcache.lock[idx]);

  // Is the block already cached?
  for(b = bcache.head[idx].next; b != &bcache.head[idx]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  int itr = idx;
  while(1) {
      itr = hash(itr + 1);
      if(itr == idx)
          continue;
      acquire(&bcache.lock[itr]);
      for(b = bcache.head[itr].prev; b != &bcache.head[itr]; b = b->prev){
          if(b->refcnt == 0) {
              b->dev = dev;
              b->blockno = blockno;
              b->valid = 0;
              b->refcnt = 1;

              release(&bcache.lock[itr]);

              b->prev->next = b->next;
              b->next->prev = b->prev;

              b->prev = &bcache.head[idx];  // 头插法
              b->next = bcache.head[idx].next;
              b->prev->next = b;
              b->next->prev = b;
              release(&bcache.lock[idx]);
              acquiresleep(&b->lock);
              return b;
          }
      }
      release(&bcache.lock[itr]);
  }
  panic("bget: no buffers");
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

  int idx = hash(b->blockno);

  releasesleep(&b->lock);

  acquire(&bcache.lock[idx]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[idx].next;
    b->prev = &bcache.head[idx];
    bcache.head[idx].next->prev = b;
    bcache.head[idx].next = b;
  }
  
  release(&bcache.lock[idx]);
}

void
bpin(struct buf *b) {
  int idx = hash(b->blockno);
  acquire(&bcache.lock[idx]);
  b->refcnt++;
  release(&bcache.lock[idx]);
}

void
bunpin(struct buf *b) {
  int idx = hash(b->blockno);
  acquire(&bcache.lock[idx]);
  b->refcnt--;
  release(&bcache.lock[idx]);
}


