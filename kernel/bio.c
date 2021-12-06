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

struct {
    struct buf head;
    struct buf tail;
} htpair[NBUCKET];

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  struct buf* bk[NBUCKET];
  struct spinlock bklock[NBUCKET];
} bcache;


int
hash(uint blockno) {
    return blockno % NBUCKET;
}

void rmnode(struct buf* node) {
    struct buf* prev = node->prev;
    prev->next = node->next;
    node->next->prev = prev;

}

void movetohead(struct buf* dmhead,struct buf* node) {
    struct buf* old = dmhead->next;
    dmhead->next = node;
    node->next = old;
    node->prev = dmhead;
    old->prev = node;
}

void movetotail(struct buf* dmtail,struct buf* node) {
    struct buf* old = dmtail->prev;
    dmtail->prev = node;
    node->prev = old;
    node->next = dmtail;
    old->next = node;
}

void
binit(void)
{
//  struct buf *b;

  initlock(&bcache.lock, "bcache");
  for (int i=0;i<NBUCKET;i++) {
      initlock(&bcache.bklock[i],"bcache_bk");

      bcache.bk[i] = &htpair[i].head;
      htpair[i].head.ishead = 1;
      bcache.bk[i]->next = &htpair[i].tail;
      htpair[i].tail.prev = bcache.bk[i];
      htpair[i].head.istail = 1;
  }

  // Create buffers
  for (int i=0;i<NBUF;i++) {
      int id = hash(i);
      struct buf* now = &bcache.buf[i];
      initsleeplock(&now->lock,"buffer");
      movetohead(bcache.bk[id],now);
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int id = hash(blockno);
  acquire(&bcache.bklock[id]);

  // found in cache?
  for (b = bcache.bk[id]->next; b && b->istail!=1;b=b->next) {
//      printf("fc:%d",b->blockno);
      if (b->dev == dev && b->blockno == blockno) {
          b->refcnt++;
          release(&bcache.bklock[id]);
          acquiresleep(&b->lock);
//          printf("bf found id:%d",id);
          return b;
      }

  }

  // not found , start evict from per bk
  struct buf* victim = 0;
  for (b=htpair[id].tail.prev;b && b->ishead!=1;b=b->prev) {
//      printf("loop1");
      if (b->refcnt ==0) {
          victim = b;
      }
  }

  if (victim) {
      victim->dev = dev;
      victim->blockno = blockno;
      victim->valid = 0;
      victim->refcnt = 1;
      release(&bcache.bklock[id]);
      acquiresleep(&victim->lock);
      return victim;
  }

  int victim_bkid = -1;

  for (int i=0;i<NBUCKET;i++) {
      if (i==id) continue;
      acquire(&bcache.bklock[i]);
      for (b=htpair[i].tail.prev;b && b->ishead!=1;b=b->prev) {
          if (b->refcnt ==0) {
              victim = b;
              victim_bkid = i;
              goto OUTER;
          }
      }
      release(&bcache.bklock[i]);
  }
OUTER:
  if (!victim) {
      release(&bcache.bklock[id]);
      panic("bget: no buffers");
  }

  rmnode(victim);

  victim->dev = dev;
  victim->blockno = blockno;
  victim->valid = 0;
  victim->refcnt = 1;
  release(&bcache.bklock[id]);
  release(&bcache.bklock[victim_bkid]);
  acquiresleep(&victim->lock);
  movetohead(&htpair[id].head,victim);
  return victim;
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
  int id = hash(b->blockno);
  acquire(&bcache.bklock[id]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // move to tail
    rmnode(b);

    movetotail(&htpair[id].tail,b);
  }
  
  release(&bcache.bklock[id]);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}

