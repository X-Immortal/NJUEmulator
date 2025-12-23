#include <utils.h>

#define RING_BUF_SIZE 16
char ring_buf[RING_BUF_SIZE][128];
int ptr = 0;
bool full = false;

static int next(int n) {
  return (n + 1) % RING_BUF_SIZE;
}

static int prev(int n) {
  return (n + RING_BUF_SIZE - 1) % RING_BUF_SIZE;
}

void rbuf_push(const char *log) {
  full = full || ptr == RING_BUF_SIZE - 1;
  strcpy(ring_buf[ptr], log);
  ptr = next(ptr);
}

void rbuf_print() {
	if (!full) {
		for (int i = 0; i < ptr - 1; i++) {
			printf("\t%s\n", ring_buf[i]);
		}
		printf("--> %s\n", ring_buf[ptr - 1]);
		return;
	}

	int cur = prev(ptr);
  for (int i = ptr; i != cur; i = next(i)) {
    printf("\t%s\n", ring_buf[i]);
  }
  printf("--> %s\n", ring_buf[cur]);
}