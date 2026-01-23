#include <common.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

void switch_fg_pcb(int n);

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

size_t serial_write(const void *buf, size_t offset, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    putch(((char *) buf)[i]);
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  AM_INPUT_KEYBRD_T kbd;
  ioe_read(AM_INPUT_KEYBRD, &kbd);
  if (kbd.keycode == AM_KEY_NONE) {
    return 0;
  } 
  
  if (kbd.keydown) {
    return snprintf((char *) buf, len, "kd %s\n", keyname[kbd.keycode]);
  } else {
    switch (kbd.keycode) {
      case AM_KEY_F1: switch_fg_pcb(1); break;
      case AM_KEY_F2: switch_fg_pcb(2); break;
      case AM_KEY_F3: switch_fg_pcb(3); break;
      default:                          break; 
    }
    return snprintf((char *) buf, len, "ku %s\n", keyname[kbd.keycode]);
  }
}

static int screen_w = 0;

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  AM_GPU_CONFIG_T screen_info;
  ioe_read(AM_GPU_CONFIG, &screen_info);
  screen_w = screen_info.width;
  return snprintf((char *) buf, len, "WIDTH:%d\nHEIGHT:%d\n", screen_info.width, screen_info.height);
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  if (screen_w == 0) {
    screen_w = io_read(AM_GPU_CONFIG).width;
  }

  offset /= sizeof(uint32_t);
  io_write(AM_GPU_FBDRAW, 
    .pixels = (void *) buf,
    .y = offset / screen_w,
    .x = offset % screen_w,
    .w = len / sizeof(uint32_t),
    .h = 1,
    .sync = true
  );
  return len;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
