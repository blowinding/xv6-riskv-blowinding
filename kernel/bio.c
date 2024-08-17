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

typedef struct {
  struct spinlock lock;
  struct buf buf[NBUCNUM];
} bcachebucket;

bcachebucket bcachemap[NBUCKET];
char lockname[NBUCKET][10];

void
binit(void)
{
  for (int i = 0; i < NBUCKET; i++)
  {
    snprintf(lockname[i], 8, "bcache%d", i);
    initlock(&bcachemap[i].lock, lockname[i]);
    struct buf *b = bcachemap[i].buf;
    for (int j = 0; j < NBUCNUM; j++)
    {
      initsleeplock(&b[j].lock, "buffer");
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint64 hash = blockno % NBUCKET;

  acquire(&bcachemap[hash].lock);
  b = bcachemap[hash].buf;
  // Is the block already cached?
  for(int i = 0; i < NBUCNUM; i++){
    if(b[i].dev == dev && b[i].blockno == blockno){
      b[i].refcnt++;
      release(&bcachemap[hash].lock);
      acquiresleep(&b[i].lock);
      return &b[i];
    }
  }

  // Not cached.
  for(int i = 0; i < NBUCNUM; i++){
    if(b[i].refcnt == 0) {
      b[i].dev = dev;
      b[i].blockno = blockno;
      b[i].valid = 0;
      b[i].refcnt = 1;
      release(&bcachemap[hash].lock);
      acquiresleep(&b[i].lock);
      return &b[i];
    }
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

  releasesleep(&b->lock);

  uint64 hash = b->blockno % NBUCKET;
  acquire(&bcachemap[hash].lock);
  b->refcnt--;
  release(&bcachemap[hash].lock);
}

void
bpin(struct buf *b) {
  uint64 hash = b->blockno % NBUCKET;
  acquire(&bcachemap[hash].lock);
  b->refcnt++;
  release(&bcachemap[hash].lock);
}

void
bunpin(struct buf *b) {
  uint64 hash = b->blockno % NBUCKET;
  acquire(&bcachemap[hash].lock);
  b->refcnt--;
  release(&bcachemap[hash].lock);
}


