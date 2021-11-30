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

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  char refcnt[PHYSTOP/PGSIZE];
} kmem;

// must be use with kmem.lock
int incr(uint64 pa,int step) {
    int pidx = pa / PGSIZE;
    if (pa > PHYSTOP || kmem.refcnt[pidx]+step < 0 ) {
        panic("kalloc:incr error");
    }
    kmem.refcnt[pidx] += step;
    return kmem.refcnt[pidx];
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
      kmem.refcnt[(uint64)p / PGSIZE] = 1;
      kfree(p);
  }

}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");


  r = (struct run*)pa;


  acquire(&kmem.lock);
  int paref = incr((uint64)pa,-1);
  release(&kmem.lock);

  if (paref==0) {
    memset(pa, 1, PGSIZE);
  }

  acquire(&kmem.lock);
  if (paref==0) {
      // Fill with junk to catch dangling refs.
      // (DONE) TODO; this is time consume , reduce the lock granularity
//      memset(pa, 1, PGSIZE);
      r->next = kmem.freelist;
      kmem.freelist = r;
  }

  release(&kmem.lock);
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
  if(r) {
      kmem.freelist = r->next;
      if (incr((uint64)r,1) !=1) {
          panic("kalloc incr error");
      }
  }

  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
