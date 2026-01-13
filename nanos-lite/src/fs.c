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

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB};

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

static size_t serial_write(const void *buf, size_t offset, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    putch(((char *) buf)[i]);
  }
  return len;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, serial_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, serial_write},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  for (int i = 3; i < NR_FILES; ++i) {
    file_table[i].read = ramdisk_read;
    file_table[i].write = ramdisk_write;
  }
  // TODO: initialize the size of /dev/fb
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
    if (fd < 0 || fd >= NR_FILES) { \
      return -1; \
    } \
    Finfo *file = &file_table[fd]; \
    if (file->open_offset >= file->size) { \
      return 0; \
    } \
    offset = file->disk_offset + file->open_offset; \
    if (file->open_offset + len > file->size) { \
      len = file->size - offset; \
    } \
  } while (0)

size_t fs_read(int fd, void *buf, size_t len) {
  size_t offset;
  check_fd();
  size_t ret = file_table[fd].read(buf, offset, len);
  file_table[fd].open_offset += len;
  return ret;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  size_t offset;
  check_fd();
  size_t ret = file_table[fd].write(buf, offset, len);
  file_table[fd].open_offset += len;
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
    default:
      return -1;
  }
  return file->open_offset = offset > file->size ? file->size : offset;
}

int fs_close(int fd) {
  if (fd < 0 || fd >= NR_FILES) {
    return -1;
  }
  return 0;
}