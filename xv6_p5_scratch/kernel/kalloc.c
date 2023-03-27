// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "spinlock.h"

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;

  /*
  P5 changes
  */
  uint free_pages; //track free pages
  uint ref_cnt[PHYSTOP / PGSIZE]; //track reference count

} kmem;

extern char end[]; // first address after kernel loaded from ELF file

// Initialize free list of physical pages.
void
kinit(void)
{
  char *p;

  initlock(&kmem.lock, "kmem");

  acquire(&kmem.lock);
  kmem.free_pages = 0;
  release(&kmem.lock);

  p = (char*)PGROUNDUP((uint)end);
  for(; p + PGSIZE <= (char*)PHYSTOP; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
// 只有ref_count为0的时候才free
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || (uint)v >= PHYSTOP) 
    panic("kfree");

  int idx = ((uint)v)/ PGSIZE;
  acquire(&kmem.lock);

  if (kmem.ref_cnt[idx] > 0) {
    kmem.ref_cnt[idx]--;
  }

  if (kmem.ref_cnt[idx] == 0) {
    // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);
  r = (struct run*)v;
  kmem.free_pages++;
  r->next = kmem.freelist;
  kmem.freelist = r;
  }
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  int idx = ((uint)r)/ PGSIZE;

  if(r){
    kmem.freelist = r->next;
    kmem.ref_cnt[idx] = 1;
    kmem.free_pages--;
  }
  release(&kmem.lock);
  return (char*)r;
}

int 
kpages(void){
  acquire(&kmem.lock);
  int num = (int)kmem.free_pages;
  release(&kmem.lock);
  return num;
}

// Set the mode to indicate whether to increase or decrease
// 0 = decrease, 1 = increase
void
changeRefer(uint v, int mode){
  if(v % PGSIZE || (char*)v < end || v >= PHYSTOP) 
  panic("changeRefer");

  int idx = (int)v / PGSIZE;
  acquire(&kmem.lock);
  if (mode){
    kmem.ref_cnt[idx]++;
  } else{
    kmem.ref_cnt[idx]--;
  }
  release(&kmem.lock);
}




