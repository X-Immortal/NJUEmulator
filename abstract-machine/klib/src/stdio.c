#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

#define BUF_SIZE 4096

typedef enum {
  LEFT_ALIGN, RIGHT_ALIGN, LEADING_ZERO, NONE,
} fill_t;

typedef enum {
  STDOUT, STR, 
} os_type_t;

typedef struct {
  os_type_t type;
  char *begin;
  char *out;
  char *end;
} ostream_t;

#ifdef __ISA_NATIVE__
typedef va_list va_list_ptr;
#define deref_va_list(args) (args)
#define ref_va_list(args) (args)
#else
typedef va_list* va_list_ptr;
#define deref_va_list(args) (*args)
#define ref_va_list(args) (&args)
#endif

struct info_t;

typedef int (*parse_t)(ostream_t *out, struct info_t *info, va_list_ptr args);

typedef struct info_t {
  parse_t parse;
  fill_t fill_type;
  int width;
  int precision;
} info_t;

static int parse_int(ostream_t *out, info_t *info, va_list_ptr args);
static int parse_str(ostream_t *out, info_t *info, va_list_ptr args);
static int parse_uint(ostream_t *out, info_t *info, va_list_ptr args);
static int parse_hex_lower(ostream_t *out, info_t *info, va_list_ptr args);
static int parse_hex_upper(ostream_t *out, info_t *info, va_list_ptr args);
static int parse_char(ostream_t *out, info_t *info, va_list_ptr args);
static int parse_ptr(ostream_t *out, info_t *info, va_list_ptr args);

static struct {
  const char *ph;
  parse_t parse;
} placeholders[] = {
  { "d" , parse_int                   },
  { "s" , parse_str                   },
  { "u" , parse_uint                  }, 
  { "x" , parse_hex_lower             },
  { "X" , parse_hex_upper             },
  { "c" , parse_char                  },
  { "p" , parse_ptr                   }, 
};

__attribute__((unused))
static const char *parse_format(const char *fmt, info_t *info) {
  info->parse = NULL;
  info->fill_type = NONE;
  info->width = info->precision = -1;

  if (*fmt == '-') {
    info->fill_type = LEFT_ALIGN;
    fmt++;
  }

  if (info->fill_type == NONE && *fmt == '0') {
    fmt++;
    info->fill_type = LEADING_ZERO;
  }

  int width = 0;
  while (*fmt >= '0' && *fmt <= '9') {
    width = width * 10 + *fmt++ - '0';
  }
  if (width == 0) {
    if (info->fill_type != NONE) {
      info->fill_type = NONE;
    }
  } else {
    if (info->fill_type == LEADING_ZERO) {
      info->precision = width;
    } else if (info->fill_type == NONE) {
      info->fill_type = RIGHT_ALIGN;
    }
    info->width = width;
  }

  if (*fmt == '.') {
    fmt++;
    int precision = 0;
    while (*fmt >= '0' && *fmt <= '9') {
      precision = precision * 10 + *fmt++ - '0';
    }
    info->precision = precision;
  }

  for (int i = 0; i < LENGTH(placeholders); i++) {
    int ph_len = strlen(placeholders[i].ph);
    if (strncmp(fmt, placeholders[i].ph, ph_len) == 0) {
      info->parse = placeholders[i].parse;
      fmt += ph_len;
      break;
    }
  }

  if (info->parse == NULL) {
    panic("Not implemented");
  }
  return fmt;
}

static void flush_stdout(ostream_t *out) {
  *out->out = '\0';
  putstr(out->begin);
  out->out = out->begin;
}

static int fill_c(ostream_t *out, int c, int n) {
  if (out->end != NULL) {
    int remaining = out->end - out->out;
    int to_fill = n;
    while (to_fill) {
      int size = to_fill < remaining ? to_fill : remaining;
      memset(out->out, c, size);
      to_fill -= size;
      out->out += size;
      if (out->out == out->end) {
        if (out->type == STDOUT) {
          flush_stdout(out);
        } else {
          break;
        }
      }
    }
  } else {
    if (out->type == STDOUT) {
      for (int i = 0; i < n; i++) {
        putch(c);
      }
    } else {
      memset(out->out, c, n);
      out->out += n;
    }
  }
  return n;
}
 
static int fill_data(ostream_t *out, const char *data, int n) {
  if (out->end != NULL) {
    int remaining = out->end - out->out;
    int to_fill = n;
    while (to_fill) {
      int size = to_fill < remaining ? to_fill : remaining;
      strncpy(out->out, data, size);
      data += size;
      to_fill -= size;
      out->out += size;
      if (out->out == out->end) {
        if (out->type == STDOUT) {
          flush_stdout(out);
        } else {
          break;
        }
      }
    }
  } else {
    if (out->type == STDOUT) {
        char temp[n + 1];
        strncpy(temp, data, n);
        temp[n] = '\0';
        putstr(temp);
    } else {
      strncpy(out->out, data, n);
      out->out += n;
    }
  }
  return n;
}

static int fill(ostream_t *out, info_t *info, const char *data, int n, int space) {
  int count = 0;
  switch (info->fill_type) {
    case RIGHT_ALIGN:
      count += fill_c(out, ' ', space);
    case NONE:          
      count += fill_data(out, data, n);
      break;
    case LEFT_ALIGN:
      count += fill_data(out, data, n);
      count += fill_c(out, ' ', space);
      break;
    default:  break;
  }

  return count;
}

#define add_zero() \
do { \
  int zero = 0; \
  switch (info->fill_type) { \
    case NONE:  case LEFT_ALIGN:  case RIGHT_ALIGN: \
      if (info->precision == 0 && arg == 0) { \
        zero = digit = 0; \
      } else { \
        zero = info->precision > digit ? info->precision - digit : 0; \
      } \
      break; \
    case LEADING_ZERO: \
      zero = info->width > digit ? info->width - digit : 0; \
      break; \
    default:  break; \
  } \
  p -= zero; \
  digit += zero; \
  memset(buf + p, '0', zero); \
} while (0)

#define fix_type_and_count_size() \
do {\
  if (info->fill_type == LEADING_ZERO && info->precision >= 0) { \
    info->fill_type = RIGHT_ALIGN; \
  } \
\
  switch (info->fill_type) { \
    case NONE:  case LEFT_ALIGN:  case RIGHT_ALIGN: \
      buf_size = info->precision > buf_size ? info->precision : buf_size; \
      break; \
    case LEADING_ZERO: \
      buf_size = info->width > buf_size ? info->width : buf_size; \
      break; \
    default:  break; \
  } \
  buf_size += 2;  \
} while (0)

#define num2str(num, base, upper, _signed) \
  do { \
    typeof(num) temp = num; \
    if (temp == 0) { \
      buf[--p] = '0'; \
    } else { \
      bool minus = (_signed) && temp < 0; \
      while (temp) { \
        int n = abs(temp % (base)); \
        if (n <= 9) { \
          buf[--p] = n + '0'; \
        } else { \
          buf[--p] = n + ((upper) ? 'A' : 'a'); \
        } \
        temp /= base; \
      } \
      if (minus)  buf[--p] = '-'; \
    } \
  } while (0)

#define parse_num_common(type, init_size, base, upper, _signed) \
  do { \
    type arg = va_arg(deref_va_list(args), type); \
    int buf_size = init_size; \
    fix_type_and_count_size(); \
    char buf[buf_size]; \
    int p = buf_size - 1; \
    buf[p] = '\0'; \
    num2str(arg, base, upper, _signed); \
    int digit = buf_size - 1 - p; \
    add_zero(); \
    int space = info->width - digit; \
    space = space >= 0 ? space : 0; \
    return fill(out, info, buf + p, digit, space); \
  } while (0)

__attribute__((unused))
static int parse_int(ostream_t *out, info_t *info, va_list_ptr args) {
  parse_num_common(int, 10, 10, false, true);
}

__attribute__((unused))
static int parse_str(ostream_t *out, info_t *info, va_list_ptr args) {
  const char *arg = va_arg(deref_va_list(args), const char *);
  int len = strlen(arg);
  if (info->precision != -1) {
    len = len > info->precision ? info->precision : len;
  }

  int space = info->width - len;
  space = space >= 0 ? space : 0;
  return fill(out, info, arg, len, space);
}

__attribute__((unused))
static int parse_uint(ostream_t *out, info_t *info, va_list_ptr args) {
  parse_num_common(unsigned int, 10, 10, false, false);
}

__attribute__((unused))
static int parse_hex(ostream_t *out, info_t *info, va_list_ptr args, bool upper) {
  parse_num_common(unsigned int, 8, 16, upper, false);
}

__attribute__((unused))
static int parse_hex_lower(ostream_t *out, info_t *info, va_list_ptr args) {
  return parse_hex(out, info, args, false);
}

__attribute__((unused))
static int parse_hex_upper(ostream_t *out, info_t *info, va_list_ptr args) {
  return parse_hex(out, info, args, true);
}

__attribute__((unused))
static int parse_char(ostream_t *out, info_t *info, va_list_ptr args) {
  char arg = va_arg(deref_va_list(args), int);

  int count = 0;
  int space = info->width - 1;
  space = space < 0 ? 0 : space;
  switch (info->fill_type) {
    case RIGHT_ALIGN:
      count += fill_c(out, ' ', space);
    case NONE:
      count += fill_c(out, arg, 1);
      break;
    case LEFT_ALIGN:
      count += fill_c(out, arg, 1);
      count += fill_c(out, ' ', space);
      break;
    default:  break;
  }

  return count;
}

__attribute__((unused))
static int parse_ptr(ostream_t *out, info_t *info, va_list_ptr args) {
  uintptr_t arg = va_arg(deref_va_list(args), uintptr_t);

  int buf_size = sizeof(uintptr_t) * 2 + 2;
  fix_type_and_count_size();

  char buf[buf_size];
  int p = buf_size - 1;
  buf[p] = '\0';
  num2str(arg, 16, false, false);
  buf[--p] = 'x';
  buf[--p] = '0';
  int digit = buf_size - 1 - p;
  
  add_zero();
  int space = info->width - digit;
  space = space >= 0 ? space : 0;
  return fill(out, info, buf + p, digit, space);
}

#define BUF_SIZE 4096

// buf version
int printf(const char *fmt, ...) {
  static char buf[BUF_SIZE] = "";

  va_list args;
  va_start(args, fmt);

  info_t info;
  int count = 0;
  ostream_t os = {STDOUT, buf, buf, buf + BUF_SIZE - 1};

  while (*fmt != '\0') {
    if (*fmt != '%') {
      *os.out++ = *fmt;
      count++;
      if (*fmt++ == '\n' || os.out == os.end) {
        flush_stdout(&os);
      }
      continue;
    }

    if (*(fmt + 1) == '%') {
      *os.out++ = '%';
      fmt += 2;
      count++;
      if (os.out == os.end) {
        flush_stdout(&os);
      }
      continue;
    }

    fmt++;
    fmt = parse_format(fmt, &info);
    count += info.parse(&os, &info, ref_va_list(args));
  }

  va_end(args);
  flush_stdout(&os);

  return count;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  info_t info;
  int count = 0;
  ostream_t os = {STR, out, out, NULL};

  while (*fmt != '\0') {
    if (*fmt != '%') {
      *os.out++ = *fmt++;
      count++;
      continue;
    }

    if (*(fmt + 1) == '%') {
      *os.out++ = '%';
      fmt += 2;
      count++;
      continue;
    }

    fmt++;
    fmt = parse_format(fmt, &info);
    count += info.parse(&os, &info, ref_va_list(ap));
  }
  *os.out = '\0';
  return count;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int ret = vsprintf(out, fmt, args);
  va_end(args);
  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int ret = vsnprintf(out, n, fmt, args);
  va_end(args);
  return ret;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  info_t info;
  int count = 0;
  ostream_t os = {STR, out, out, out + n - 1};

  while (*fmt != '\0') {
    if (*fmt != '%') {
      if (os.out != os.end) {
        *os.out++ = *fmt++;
      }
      count++;
      continue;
    }

    if (*(fmt + 1) == '%') {
      if (os.out != os.end) {
        *os.out++ = '%';
      }
      fmt += 2;
      count++;
      continue;
    }

    fmt++;
    fmt = parse_format(fmt, &info);
    count += info.parse(&os, &info, ref_va_list(ap));
  }
  *os.out = '\0';
  return count;
}

#endif
