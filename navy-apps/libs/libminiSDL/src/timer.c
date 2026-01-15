#include <NDL.h>
#include <sdl-timer.h>
#include <stdio.h>
#include <assert.h>

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
  extern uint32_t SDL_start_time;
  return NDL_GetTicks() - SDL_start_time;
}

void SDL_Delay(uint32_t ms) {
  if (ms == 0) return;
  uint32_t start = SDL_GetTicks();
  while (SDL_GetTicks() - start < ms);
}
