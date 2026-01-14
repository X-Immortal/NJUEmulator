#include <stdio.h>
#include <fixedptc.h>
#include <assert.h>

int main() {
  fixedpt a = fixedpt_fromint(1);
  fixedpt b = fixedpt_rconst(0.5);
  fixedpt r = fixedpt_div(a, b);
  printf("r=%x, 2=%x\n", r, (fixedpt) fixedpt_fromint(2));
  assert(r == fixedpt_fromint(2));
  return 0;
}
