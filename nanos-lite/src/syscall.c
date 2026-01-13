#include <common.h>
#include "syscall.h"
#include <fs.h>
#include <timer.h>

#define write(buf, len) do { for (size_t _i = 0; _i < (len); _i++) { putch(((char *) (buf))[_i]); } } while (0)
#define is_file_opr(sysc_no) ( \
  sysc_no == SYS_open  || \
  sysc_no == SYS_write || \
  sysc_no == SYS_read  || \
  sysc_no == SYS_lseek || \
  sysc_no == SYS_close    \
)

__attribute__((unused))
static const char *syscall_name[] = {
  "SYS_exit",
  "SYS_yield",
  "SYS_open",
  "SYS_read",
  "SYS_write",
  "SYS_kill",
  "SYS_getpid",
  "SYS_close",
  "SYS_lseek",
  "SYS_brk",
  "SYS_fstat",
  "SYS_time",
  "SYS_signal",
  "SYS_execve",
  "SYS_fork",
  "SYS_link",
  "SYS_unlink",
  "SYS_wait",
  "SYS_times",
  "SYS_gettimeofday",
};


void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;

  switch (a[0]) {
    case SYS_yield: yield(); c->GPRx = 0; break;
    case SYS_exit:  halt(a[1]);           break;
    case SYS_brk: c->GPRx = 0;            break;
    case SYS_open:
      c->GPRx = fs_open((const char *) a[1], (int) a[2], (int) a[3]);
      break;
    case SYS_write:
      c->GPRx = fs_write((int) a[1], (const void *) a[2], (size_t) a[3]);
      break;
    case SYS_read:
      c->GPRx = fs_read((int) a[1], (void *) a[2], (size_t) a[3]);
      break;
    case SYS_lseek:
      c->GPRx = fs_lseek((int) a[1], (size_t) a[2], (int) a[3]);
      break;
    case SYS_close:
      c->GPRx = fs_close((int) a[1]);
      break;
    case SYS_gettimeofday:
      c->GPRx = tm_gettimeofday((struct timeval *) a[1], (struct timezone *) a[2]);
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }

  // if (is_file_opr(a[0])) {
  //   const char *name = a[0] == SYS_open ? (const char *) a[1] : get_filename(a[1]);
  //   printf("syscall %s occured, id=%d, args=[%s, %u, %u], ret=%d\n",
  //         syscall_name[a[0]], a[0], name, a[2], a[3], c->GPRx);
  // } else {
  //   printf("syscall %s occured, id=%d, args=[%u, %u, %u], ret=%d\n",
  //         syscall_name[a[0]], a[0], a[1], a[2], a[3], c->GPRx);
  // }
}
