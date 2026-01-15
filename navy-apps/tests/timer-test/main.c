#include <stdio.h>
#include <NDL.h>

#define TEST_TIMES 100

int main() {
	NDL_Init(0);
	uint32_t start = NDL_GetTicks();
	for (size_t i = 1; i <= TEST_TIMES; ++i) {
		while (NDL_GetTicks() - start < i * 500);
		printf("passed %d * 0.5 seconds\n", i);
	}
	NDL_Quit();
}