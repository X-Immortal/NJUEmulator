#include <stdio.h>
#include <fixedptc.h>
#include <assert.h>

int main() {
  fixedpt a = fixedpt_fromint(-2);
  fixedpt b = fixedpt_fromint(3);
  assert(fixedpt_floor(a) == a && fixedpt_ceil(a) == a);
  assert(fixedpt_floor(b) == b && fixedpt_ceil(b) == b);
  return 0;
}
