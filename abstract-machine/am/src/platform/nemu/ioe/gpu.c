#include <am.h>
#include <nemu.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

static uint32_t width;
static uint32_t height;
static uint32_t vmemsz;

void __am_gpu_init() {
  width = inl(VGACTL_ADDR) >> 16;
  height = inl(VGACTL_ADDR) & ((1 << 16) - 1);
  vmemsz = width * height * sizeof(uint32_t);

  int i;
  int w = width;  // TODO: get the correct width
  int h = height;  // TODO: get the correct height
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  for (i = 0; i < w * h; i ++) fb[i] = i;
  outl(SYNC_ADDR, 1);
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = width, .height = height,
    .vmemsz = vmemsz
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  uint32_t *pixels = ctl->pixels;
  uint32_t vmem_base = FB_ADDR + ctl->y * width + ctl->x;
  for (int i = 0; i < ctl->h; i++) {
    for (int j = 0; j < ctl->w; j++) {
      outl(vmem_base + i * width + j, pixels[i * ctl->w + j]);
    }
  }
  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  } 
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
