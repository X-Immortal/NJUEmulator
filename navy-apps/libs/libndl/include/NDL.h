#ifndef __NDL_H__
#define __NDL_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef union {
#if __BYTE_ORDER == __LITTLE_ENDIAN 
  struct { uint8_t b, g, r, a; };
#else
  struct { uint8_t a, r, g, b; };
#endif
  uint32_t val;
} NDL_Color;

int NDL_Init(uint32_t flags);
void NDL_Quit();
uint32_t NDL_GetTicks();
void NDL_OpenCanvas(int *w, int *h);
int NDL_PollEvent(char *buf, int len);
void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h);
void NDL_OpenAudio(int freq, int channels, int samples);
void NDL_CloseAudio();
int NDL_PlayAudio(void *buf, int len);
int NDL_QueryAudio();

#ifdef __cplusplus
}
#endif

#endif
