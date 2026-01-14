#include <stdio.h>
#include <fixedptc.h>
#include <assert.h>

int main() {
  fixedpt b = fixedpt_rconst(1.33);
  fixedpt r = fixedpt_floor(b);
  printf("r=%x, 1=%x\n", r, (fixedpt) fixedpt_fromint(1));
  assert(r == fixedpt_fromint(1));
  return 0;
}
