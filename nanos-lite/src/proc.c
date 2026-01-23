#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;
PCB *fg_pcb = NULL;

void switch_boot_pcb() {
  current = &pcb_boot;
}

void switch_fg_pcb(int n) {
  if (n > 0 && n < MAX_NR_PROC) {
    fg_pcb = &pcb[n];
  }
}

void hello_fun(void *arg) {
  int j = 1;
  int i = 0;
  while (1) {
    if (i < 250) {
      i++;
    } else {
      Log("Hello World from Nanos-lite with arg '%c' for the %dth time!", (uintptr_t)arg, j);
      i = 0;
      j ++;
    }
    yield();
  }
}

void init_proc() {
  context_kload(&pcb[0], hello_fun, (void *) 'a');
  char *argv1[] = { "/bin/nterm", NULL };
  context_uload(&pcb[1], argv1[0], argv1, NULL);
  char *argv2[] = { "/bin/nterm", NULL };
  context_uload(&pcb[2], argv2[0], argv2, NULL);
  char *argv3[] = { "/bin/nterm", NULL };
  context_uload(&pcb[3], argv3[0], argv3, NULL);
  switch_boot_pcb();

  fg_pcb = &pcb[1];

  Log("Initializing processes...");

  // load program here
  // naive_uload(NULL, "/bin/nterm");
}

Context* schedule(Context *prev) {
  current->cp = prev;
  current = (current == &pcb[0] ? fg_pcb : &pcb[0]);
  return current->cp;
}
