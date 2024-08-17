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

int maxcpuid = 0;
struct spinlock cpulock;

typedef struct {
  struct spinlock lock;
  struct run *freelist;
  uint64 num;
} kmem;
char namebuf[NCPU][10];
kmem kmemlist[NCPU];

void kinitcpu() {
  initlock(&cpulock, "maxcpuid");
}

void
kinit()
{
  int cpu;
  push_off();
  cpu = cpuid();
  pop_off();
  acquire(&cpulock);
  maxcpuid = maxcpuid > cpu ? maxcpuid : cpu;
  release(&cpulock);
  snprintf(namebuf[cpu], 6, "kmem%d", cpu);
  initlock(&kmemlist[cpu].lock, namebuf[cpu]);
  if (!cpu)
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
  int cpu;
  push_off();
  cpu = cpuid();
  pop_off();

  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmemlist[cpu].lock);
  r->next = kmemlist[cpu].freelist;
  kmemlist[cpu].freelist = r;
  kmemlist[cpu].num++;
  release(&kmemlist[cpu].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  int cpu;
  push_off();
  cpu = cpuid();
  pop_off();

  struct run *r;

  acquire(&kmemlist[cpu].lock);
  r = kmemlist[cpu].freelist;
  if(r) {
    kmemlist[cpu].freelist = r->next;
    kmemlist[cpu].num--;
  } else {
    release(&kmemlist[cpu].lock);
    struct run *next;
    uint64 freenum = 0;
    for (int i = 0; i <= maxcpuid; i++)
    {
      if (i != cpu) {
        acquire(&kmemlist[i].lock);
        if (kmemlist[i].freelist) {
          freenum = kmemlist[i].num / (maxcpuid + 1);
          // printf("freenum: %d\n", freenum);
          freenum = freenum > 0 ? freenum : 1;
          uint64 tmp = freenum;
          kmemlist[i].num -= freenum;
          r = kmemlist[i].freelist;
          struct run *h = r;
          while (tmp > 1) {
            tmp--;
            h = h->next;
          }
          next = h;
          kmemlist[i].freelist = h->next;
        }
        release(&kmemlist[i].lock);
        if (freenum)
          break;
      }
    }
    acquire(&kmemlist[cpu].lock);
    if (freenum) {
      // printf("next is %p, freenum is %d\n", next, freenum);
      next->next = kmemlist[cpu].freelist;
      kmemlist[cpu].freelist = r->next;
      kmemlist[cpu].num += (freenum - 1);
    }
  }
  release(&kmemlist[cpu].lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
