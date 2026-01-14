#include <NDL.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#define ERROR_BUF_SIZE 1024

uint32_t SDL_start_time;
static char error_buf[ERROR_BUF_SIZE];

int SDL_Init(uint32_t flags) {
  int ret = NDL_Init(flags);
  SDL_start_time = NDL_GetTicks();
  return ret;
}

void SDL_Quit() {
  NDL_Quit();
}

char *SDL_GetError() {
  return error_buf;
}

int SDL_SetError(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int ret = vsnprintf(error_buf, ERROR_BUF_SIZE, fmt, args);
  va_end(args);
  return ret;
}

int SDL_ShowCursor(int toggle) {
  printf("%s not implemented!\n", __func__);
  assert(0);
  return 0;
}

void SDL_WM_SetCaption(const char *title, const char *icon) {
  printf("%s not implemented!\n", __func__);
  assert(0);
}
