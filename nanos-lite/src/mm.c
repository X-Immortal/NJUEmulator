#include <memory.h>
#include <proc.h>

static void *pf = NULL;

void* new_page(size_t nr_page) {
  size_t size = nr_page * PGSIZE;
  if (pf + size > heap.end) {
    panic("Out of Memory!");
  }
  void *page = pf;
  pf += size;
  return page;
}

#ifdef HAS_VME
static void* pg_alloc(int n) {
  assert(n % PGSIZE == 0);
  void *page = new_page(n / PGSIZE);
  memset(page, 0, n);
  return page;
}
#endif

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
  if (current->max_brk == 0) {
    current->max_brk = ROUNDDOWN(brk, PGSIZE);
    return 0;
  }
  if (brk <= current->max_brk) return 0;

  uintptr_t new_space = ROUNDUP(brk - current->max_brk, PGSIZE);
  int nr_page = new_space / PGSIZE;
  void *page = new_page(nr_page);
  map(&current->as, 
      (void *) (current->max_brk),
      (void *) ((uintptr_t) page),
      0);
  // for (int i = 0; i < nr_page; ++i) {
  //   map(&current->as, 
  //       (void *) (current->max_brk + i * PGSIZE),
  //       (void *) ((uintptr_t) page + i * PGSIZE),
  //       0);
  // }
  current->max_brk += new_space;
  return 0;
}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
