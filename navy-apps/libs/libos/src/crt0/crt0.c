#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

int main(int argc, char *argv[], char *envp[]);
extern char **environ;
void call_main(uintptr_t *args) {
  int argc = *(int *) args;
  char **argv = (char **) ((int *) args + 1);
  char **envp = argv + argc + 1;
  if (*envp || !environ) {
    environ = envp;
  } else {
    envp = environ;
  }
  exit(main(argc, argv, envp));
  assert(0);
}
