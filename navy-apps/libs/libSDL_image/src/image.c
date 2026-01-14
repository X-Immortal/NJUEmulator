#define SDL_malloc  malloc
#define SDL_free    free
#define SDL_realloc realloc

#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"
#include <stdio.h>

SDL_Surface* IMG_Load_RW(SDL_RWops *src, int freesrc) {
  assert(src->type == RW_TYPE_MEM);
  assert(freesrc == 0);
  return NULL;
}

SDL_Surface* IMG_Load(const char *filename) {
  FILE *image = fopen(filename, "rb");
  int ret = fseek(image, 0, SEEK_END);
  assert(ret == 0);
  int size = (int) ftell(image);
  uint8_t *buf = SDL_malloc(size);
  assert(buf);
  ret = fseek(image, 0, SEEK_SET);
  assert(ret == 0);
  ret = fread(buf, size, 1, image);
  assert(ret == size);
  SDL_Surface *surface = STBIMG_LoadFromMemory((const unsigned char *) buf, size);
  SDL_free(buf);
  fclose(image);
  return surface;
}

int IMG_isPNG(SDL_RWops *src) {
  return 0;
}

SDL_Surface* IMG_LoadJPG_RW(SDL_RWops *src) {
  return IMG_Load_RW(src, 0);
}

char *IMG_GetError() {
  return SDL_GetError();
}
