#include <stdio.h>
#include <fixedptc.h>
#include <assert.h>

int main() {
  fixedpt b = fixedpt_rconst(1.33);
  fixedpt r = fixedpt_ceil(b);
  assert(r == fixedpt_fromint(2));
  return 0;
}
