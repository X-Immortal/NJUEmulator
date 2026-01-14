#include <stdio.h>
#include <fixedptc.h>

int main() {
  fixedpt a = fixedpt_rconst(0.8);
  fixedpt b = fixedpt_rconst(0.4);
  fixedpt r = fixedpt_mul(a, b);
  printf("%f * %f = %f\n", 0.8, 0.4, fixedpt_tofloat(r));
  return 0;
}
