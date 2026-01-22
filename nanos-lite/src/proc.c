#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  // int i = 0;
  while (1) {
    // if (i < 1000000) {
      // i++;
    // } else {
      Log("Hello World from Nanos-lite with arg '%c' for the %dth time!", (uintptr_t)arg, j);
      // i = 0;
    // }
    j ++;
    yield();
  }
}

void init_proc() {
  context_kload(&pcb[0], hello_fun, (void *) 'a');
  char *argv[] = { "/bin/pal", NULL };
  char *envp[] = { NULL };
  context_uload(&pcb[1], "/bin/pal", argv, envp);
  switch_boot_pcb();

  Log("Initializing processes...");

  // load program here
  // naive_uload(NULL, "/bin/nterm");
}

Context* schedule(Context *prev) {
  current->cp = prev;
  current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
  return current->cp;
}
