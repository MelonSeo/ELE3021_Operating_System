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
  int refcount[(PHYSTOP - KERNBASE) / PGSIZE];
} kmem;

void increfcount(void *pa) {
  if ((uint64)pa >= PHYSTOP || (uint64)pa < (uint64)end)
    panic("increase ref count");
  acquire(&kmem.lock);
  kmem.refcount[PA2INDEX(pa)]++;
  release(&kmem.lock);
}

void decrefcount(void *pa) {
   if ((uint64)pa >= PHYSTOP || (uint64)pa < (uint64)end)
    panic("decrease ref count");
  acquire(&kmem.lock);
  kmem.refcount[PA2INDEX(pa)]--;
  release(&kmem.lock);
}

int getrefcount(void *pa) {
  if ((uint64)pa >= PHYSTOP ||  (uint64)pa < (uint64)end)
    panic("get ref count");
  int count;
  acquire(&kmem.lock);
  count = kmem.refcount[PA2INDEX(pa)];
  release(&kmem.lock);
  return count;
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
    kmem.refcount[PA2INDEX(p)] = 1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&kmem.lock);
  kmem.refcount[PA2INDEX(pa)]--;
  if(kmem.refcount[PA2INDEX(pa)] > 0) { //refcount가 0보다 크면 free X
    release(&kmem.lock);
    return;
  }
  release(&kmem.lock);
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r; //freelist

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk
    acquire(&kmem.lock);
    kmem.refcount[PA2INDEX((void*)r)] = 1; //void* casting
    release(&kmem.lock);
  }
  return (void*)r;
}
