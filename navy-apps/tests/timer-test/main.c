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
			if (cur.tv_usec - start.tv_sec >= i * 0.5) {
				break;
			}
		}
		printf("passed %.1f seconds\n", i * 0.5);
	}
}