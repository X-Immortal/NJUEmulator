#include <fs.h>

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
  size_t open_offset;
} Finfo;

enum {
  FD_STDIN, FD_STDOUT, FD_STDERR, 
  FD_EVENTS, FD_DISPINFO, FD_FB,
};

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t serial_write(const void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len);
size_t dispinfo_read(void *buf, size_t offset, size_t len);
size_t fb_write(const void *buf, size_t offset, size_t len);

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", SIZE_MAX, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", SIZE_MAX, 0, invalid_read, serial_write},
  [FD_STDERR] = {"stderr", SIZE_MAX, 0, invalid_read, serial_write},
  [FD_EVENTS] = {"/dev/events", SIZE_MAX, 0, events_read, invalid_write},
  [FD_DISPINFO] = {"/proc/dispinfo", SIZE_MAX, 0, dispinfo_read, invalid_write},
  [FD_FB] = {"/dev/fb", 0, 0, invalid_read, fb_write},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  // TODO: initialize the size of /dev/fb
  AM_GPU_CONFIG_T screen_info;
  ioe_read(AM_GPU_CONFIG, &screen_info);
  file_table[FD_FB].size = screen_info.vmemsz;
}

int fs_open(const char *pathname, int flags, int mode) {
  for (int i = 0; i < NR_FILES; ++i) {
    if (strcmp(pathname, file_table[i].name) == 0) {
      file_table[i].open_offset = 0;
      return i;
    }
  }
  panic("file not exists, should not reach here");
}

#define check_fd() \
  do { \
    Finfo *file = &file_table[fd]; \
    if (file->open_offset >= file->size) { \
      return 0; \
    } \
    offset = file->disk_offset + file->open_offset; \
    if (file->open_offset + len > file->size) { \
      len = file->size - file->open_offset; \
    } \
  } while (0)

size_t fs_read(int fd, void *buf, size_t len) {
  if (fd < 0 || fd >= NR_FILES) {
    return -1;
  }

  size_t offset = 0;
  size_t ret;
  check_fd();
  if (file_table[fd].read == NULL) {
    ret = ramdisk_read(buf, offset, len);
  } else {
    ret = file_table[fd].read(buf, offset, len);
  }
  file_table[fd].open_offset += ret;
  return ret;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  if (fd < 0 || fd >= NR_FILES) {
    return -1;
  }

  size_t offset = 0;
  size_t ret;
  check_fd();
  if (file_table[fd].read == NULL) {
    ret = ramdisk_write(buf, offset, len);
  } else {
    ret = file_table[fd].write(buf, offset, len);
  }
  file_table[fd].open_offset += ret;
  return ret;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  if (fd < 0 || fd >= NR_FILES) {
    return -1;
  }

  Finfo *file = &file_table[fd];
  switch (whence) {
    case SEEK_SET:
      break;
    case SEEK_CUR:
      offset += file->open_offset;
      break;
    case SEEK_END:
      offset += file->size;
      break;
    default:
      return -1;
  }
  file->open_offset = offset > file->size ? file->size : offset;
  printf("offset=%zu, size=%zu, open_offset=%zu\n", offset, file->size, file->open_offset);
  return file->open_offset;
}

int fs_close(int fd) {
  if (fd < 0 || fd >= NR_FILES) {
    return -1;
  }
  return 0;
}

const char *get_filename(int fd) {
  if (fd < 0 || fd >= NR_FILES) {
    return "invalid file";
  }
  return file_table[fd].name;
}