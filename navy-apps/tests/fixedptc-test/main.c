#include <stdio.h>
#include <fixedptc.h>
#include <assert.h>

int main() {
  fixedpt a = fixedpt_rconst(0.8);
  fixedpt b = fixedpt_rconst(0.4);
  fixedpt r = fixedpt_mul(a, b);
  assert(r == fixedpt_rconst(0.32));
  return 0;
}
