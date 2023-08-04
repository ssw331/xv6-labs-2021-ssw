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
  struct spinlock lock[NCPU];
  struct run *freelist[NCPU];
} kmem;

void
kinit()
{
  for(int i = 0; i < NCPU; i++) {
      initlock(&kmem.lock[i], "kmem");
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  uint64 freesz = (uint64)pa_end - (uint64)pa_start;
  for(int i = 0; i < NCPU; i++) {
      for(; p + PGSIZE <= end + freesz * (i + 1) / NCPU; p += PGSIZE) {
          struct run *r = (struct run*)p;
          acquire(&kmem.lock[i]);  // 让当前cpu获得空闲内存
          r->next = kmem.freelist[i];
          kmem.freelist[i] = r;
          release(&kmem.lock[i]);
      }
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

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();  // turn interrupt off to make the cpuid() safe
  int c_id = cpuid();  // 获得当前cpuid
  acquire(&kmem.lock[c_id]);  // 让当前cpu获得空闲内存
  r->next = kmem.freelist[c_id];
  kmem.freelist[c_id] = r;
  release(&kmem.lock[c_id]);
  pop_off();  // turn interrupt on
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int c_id = cpuid();
  acquire(&kmem.lock[c_id]);
  r = kmem.freelist[c_id];
  if(r)
    kmem.freelist[c_id] = r->next;
  else {
      int borrow;
      for(borrow = 0; borrow < NCPU; borrow++) {
          if(borrow == c_id)
              continue;
          acquire(&kmem.lock[borrow]);
          r = kmem.freelist[borrow];
          if(kmem.freelist[borrow]) {
              kmem.freelist[borrow] = r->next;
              release(&kmem.lock[borrow]);
              break;
          }
          release(&kmem.lock[borrow]);
      }
  }
  release(&kmem.lock[c_id]);
  pop_off();

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
