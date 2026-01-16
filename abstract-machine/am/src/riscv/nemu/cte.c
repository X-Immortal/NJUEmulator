#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  // for (int i = 0; i < NR_REGS; i++) {
  //   printf("x%d: %u\n", i, c->gpr[i]);
  // }
  // printf("mcause: %u\n", c->mcause);
  // printf("mstatus: %u\n", c->mstatus);
  // printf("mepc: %u\n", c->mepc);

  if (user_handler) {
    Event ev = {0};
    switch (c->mcause) {
      case MCAUSE_ECALL: 
        if (c->GPR1 == (uintptr_t) -1) {
          ev.event = EVENT_YIELD;
        } else {
          ev.event = EVENT_SYSCALL;
        }
        c->mepc += 4;
        break;
      default: ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }

  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  Context *context = (Context *) kstack.end - 1;
  context->mepc = (uintptr_t) entry;
  context->mstatus = (uintptr_t) 0x1800; // set mstatus to pass difftest
  context->GPR2 = (uintptr_t) arg;
  return context;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
