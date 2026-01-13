#include <stdio.h>
#include <sys/time.h>

#define TEST_TIMES 100

int main() {
	struct timeval start;
	gettimeofday(&start, NULL);
	struct timeval cur;
	for (size_t i = 1; i <= TEST_TIMES; ++i) {
		while (1) {
			gettimeofday(&cur, NULL);
			if (cur.tv_usec - start.tv_sec >= i * 500000) {
				break;
			}
		}
		printf("passed %d * 0.5 seconds\n", i);
	}
}