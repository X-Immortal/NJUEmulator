#include <NDL.h>
#include <stdio.h>
#include <assert.h>

uint32_t SDL_start_time;

int SDL_Init(uint32_t flags) {
  int ret = NDL_Init(flags);
  SDL_start_time = NDL_GetTicks();
  return ret;
}

void SDL_Quit() {
  NDL_Quit();
}

char *SDL_GetError() {
  return "Navy does not support SDL_GetError()";
}

int SDL_SetError(const char* fmt, ...) {
  printf("%s not implemented!\n", __func__);
  assert(0);
  return -1;
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
