#include <stdio.h>
#include <fixedptc.h>
#include <assert.h>

int main() {
  fixedpt a = fixedpt_rconst(0.8);
  fixedpt b = fixedpt_rconst(0.4);
  fixedpt r = fixedpt_mul(a, b);
  printf("r=%d, 0.32=%d\n", r, fixedpt_rconst(0.32));
  assert(r == fixedpt_rconst(0.32));
  return 0;
}
