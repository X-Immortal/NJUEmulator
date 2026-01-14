#include <NDL.h>
#include <SDL.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define keyname(k) #k,
#define NR_KEYS (sizeof(keyname) / sizeof(*keyname))

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

int SDL_PushEvent(SDL_Event *ev) {
  printf("%s not implemented!\n", __func__);
  assert(0);
  return 0;
}

int SDL_PollEvent(SDL_Event *ev) {
  printf("%s not implemented!\n", __func__);
  assert(0);
  return 0;
}

int SDL_WaitEvent(SDL_Event *event) {
  char buf[20];
  char key[20];
  int ret;
  while (1) {
    if (NDL_PollEvent(buf, sizeof(buf))) {
      if (sscanf(buf, "kd %s", key) == 1) {
        event->type = SDL_KEYDOWN;
      } else if (sscanf(buf, "ku %s", key) == 1) {
        event->type = SDL_KEYUP;
      }

      for (int i = 0; i < NR_KEYS; ++i) {
        if (strcmp(key, keyname[i]) == 0) {
          event->key.keysym.sym = i;
          return 1;
        }
      }
    }
  }
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  printf("%s not implemented!\n", __func__);
  assert(0);
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  printf("%s not implemented!\n", __func__);
  assert(0);
  return NULL;
}
