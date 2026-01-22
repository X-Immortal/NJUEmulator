#include <am.h>
#include <nemu.h>
#include <klib.h>

static AddrSpace kas = {};
static void* (*pgalloc_usr)(int) = NULL;
static void (*pgfree_usr)(void*) = NULL;
static int vme_enable = 0;

static Area segments[] = {      // Kernel memory mappings
  NEMU_PADDR_SPACE
};

#define USER_SPACE RANGE(0x40000000, 0x80000000)
#define VPN1(va) ((va) >> 22)
#define VPN2(va) (((va) >> 12) & 0x000003FF)
#define PPN(pa) ((pa) >> 12)
#define MAKE_PTE(ppn) ((ppn) << 10)
#define EXTRACT_PTE(pte) ((pte) >> 10)
#define MERGE_ADDR(pno, pgoff) (((pno) << 12) | (pgoff))

static inline void set_satp(void *pdir) {
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  asm volatile("csrw satp, %0" : : "r"(mode | ((uintptr_t)pdir >> 12)));
}

static inline uintptr_t get_satp() {
  uintptr_t satp;
  asm volatile("csrr %0, satp" : "=r"(satp));
  return satp << 12;
}

bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  pgalloc_usr = pgalloc_f;
  pgfree_usr = pgfree_f;

  kas.ptr = pgalloc_f(PGSIZE);
  kas.pgsize = PGSIZE;

  int i;
  for (i = 0; i < LENGTH(segments); i ++) {
    void *va = segments[i].start;
    for (; va < segments[i].end; va += PGSIZE) {
      map(&kas, va, va, 0);
    }
  }

  set_satp(kas.ptr);
  vme_enable = 1;

  return true;
}

void protect(AddrSpace *as) {
  PTE *updir = (PTE*)(pgalloc_usr(PGSIZE));
  as->ptr = updir;
  as->area = USER_SPACE;
  as->pgsize = PGSIZE;
  // map kernel space
  memcpy(updir, kas.ptr, PGSIZE);
}

void unprotect(AddrSpace *as) {
}

void __am_get_cur_as(Context *c) {
  c->pdir = (vme_enable ? (void *)get_satp() : NULL);
}

void __am_switch(Context *c) {
  if (vme_enable && c->pdir != NULL) {
    set_satp(c->pdir);
  }
}

void map(AddrSpace *as, void *va, void *pa, int prot) {
  PTE *updir = (PTE *) as->ptr;
  uintptr_t vpn1 = VPN1((uintptr_t) va);
  PTE *upt = (PTE *) MERGE_ADDR(EXTRACT_PTE(updir[vpn1]), 0);
  if (upt == NULL) {
    upt = pgalloc_usr(PGSIZE);
    updir[vpn1] = MAKE_PTE(PPN((uintptr_t) upt));
  }
  upt[VPN2((uintptr_t) va)] = MAKE_PTE(PPN((uintptr_t) pa));
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  Context *context = (Context *) kstack.end - 1;
  context->mepc = (uintptr_t) entry;
  context->mstatus = (uintptr_t) 0x1808; // set mstatus to pass difftest
  context->pdir = as->ptr;
  return context;
}
