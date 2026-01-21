#include <unistd.h>
#include <stdio.h>

void *_sbrk(intptr_t increment);

int main() {
  // write(1, "Hello World!\n", 13);
  // printf("1\n");
  // int i = 2;
  // volatile int j = 0;
  // while (1) {
  //   j ++;
  //   if (j == 10000) {
  //     printf("Hello World from Navy-apps for the %dth time!\n", i ++);
  //     j = 0;
  //   }
  // }
  void *start = _sbrk(0);
  void *end = _sbrk(4096);
  ((char *) start)[0] = 'A';
  ((char *) start)[1] = 'B';
  assert(((char *) start)[0] + 1 == ((char *) start)[1]);
  return 0;
}
