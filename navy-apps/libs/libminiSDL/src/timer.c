#include <NDL.h>
#include <sdl-timer.h>
#include <stdio.h>

SDL_TimerID SDL_AddTimer(uint32_t interval, SDL_NewTimerCallback callback, void *param) {
  printf("%s not implemented!\n", __func__);
  assert(0);
  return NULL;
}

int SDL_RemoveTimer(SDL_TimerID id) {
  printf("%s not implemented!\n", __func__);
  assert(0);
  return 1;
}

uint32_t SDL_GetTicks() {
  printf("%s not implemented!\n", __func__);
  assert(0);
  return 0;
}

void SDL_Delay(uint32_t ms) {
  printf("%s not implemented!\n", __func__);
  assert(0);
}
