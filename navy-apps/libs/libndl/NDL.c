#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>

static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0;
static int canvas_w = 0, canvas_h = 0;
static int canvas_x = 0, canvas_y = 0;

uint32_t NDL_GetTicks() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
  return tv.tv_usec / 1000;
}

int NDL_PollEvent(char *buf, int len) {
  FILE *kbd = fopen("/dev/events", "r");
  char *ret = fgets((void *) buf, len, kbd);
  fclose(kbd);
  return ret ? 1 : 0;
}

void NDL_OpenCanvas(int *w, int *h) {
  if (*w == 0 && *h == 0) {
    *w = screen_w;
    *h = screen_h;
  }
  canvas_w = *w > screen_w ? screen_w : *w;
  canvas_h = *h > screen_h ? screen_h : *h;
  canvas_x = (screen_w - canvas_w) / 2;
  canvas_y = (screen_h - canvas_h) / 2;

  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  }
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  FILE *screen = fopen("/dev/fb", "wb");
  int ret;
  if (w == screen_w) {
    ret = fwrite(pixels, sizeof(*pixels), w * h, screen);
    assert(ret == w * h);
  } else {
    long base = (canvas_y + y) * screen_w + (canvas_x + x);
    for (int i = 0; i < h; ++i) {
      ret = fseek(screen, (base + i * screen_w) * sizeof(uint32_t), SEEK_SET);
      assert(ret == 0);
      ret = fwrite(pixels + i * w, sizeof(*pixels), w, screen);
      assert(ret == w);
    }
  }
  fclose(screen);
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

static void init_screen() {
  FILE *screen_info = fopen("/proc/dispinfo", "r");
  int ret = fscanf(screen_info, " WIDTH : %d HEIGHT : %d", &screen_w, &screen_h);
  assert(ret == 2);
  fclose(screen_info);
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }
  init_screen();
  return 0;
}

void NDL_Quit() {
}
