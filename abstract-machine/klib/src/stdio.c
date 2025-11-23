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
  char *out;
  char *end;
} ostream_t;

struct info_t;

typedef int (*parse_t)(ostream_t *out, struct info_t *info, va_list *args);

typedef struct info_t {
  parse_t parse;
  fill_t fill_type;
  int width;
  int precision;
} info_t;

static int parse_int(ostream_t *out, info_t *info, va_list *args);
static int parse_str(ostream_t *out, info_t *info, va_list *args);

static struct {
  const char *ph;
  parse_t parse;
} placeholders[] = {
  { "d" , parse_int },
  { "s" , parse_str },
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

static int fill_c(ostream_t *out, int c, int n) {
  if (out->type == STDOUT) {
    for (int i = 0; i < n; i++) {
      putch(c);
    }
    return n;
  }
  if (out->type == STR) {
    if (out->end != NULL) {
      if (out->out > out->end) return 0;
      if (out->end - out->out + 1 < n) {
        memset(out->out, c, out->end - out->out + 1);
        out->out = out->end;
        return out->end - out->out + 1;
      }
    }
    memset(out->out, c, n);
    out->out += n;
    return n;
  }
  return -1;
}
 
static int fill_data(ostream_t *out, const char *data, int n) {
  if (out->type == STDOUT) {
    char temp[n + 1];
    strncpy(temp, data, n);
    temp[n] = '\0';
    putstr(temp);
    return n;
  }
  if (out->type == STR) {
    if (out->end != NULL) {
      if (out->out > out->end) return 0;
      if (out->end - out->out + 1 < n) {
        strncpy(out->out, data, out->end - out->out + 1);
        out->out = out->end;
        return out->out - out->end + 1;
      }
    }
    strncpy(out->out, data, n);
    out->out += n;
    return n;
  }
  return -1;
}

__attribute__((unused))
static int parse_int(ostream_t *out, info_t *info, va_list *args) {
  int arg = va_arg(*args, int);
  if (arg == INT32_MIN) {
    panic("-2147483648");
  }
  int arg_temp = arg < 0 ? -arg : arg;

  if (info->fill_type == LEADING_ZERO && info->precision >= 0) {
    info->fill_type = RIGHT_ALIGN;
  }

  int temp_len = 10; // int max has 10 digits
  switch (info->fill_type) {
    case NONE:  case LEFT_ALIGN:  case RIGHT_ALIGN:
      temp_len = info->precision > temp_len ? info->precision : temp_len;
      break;
    case LEADING_ZERO:
      temp_len = info->width > temp_len ? info->width : temp_len;
      break;
    default:  break;
  }
  temp_len += 2;  // one for '\0', one for '-'

  char temp[temp_len];
  int p = temp_len - 1;
  temp[p] = '\0';
  while (arg_temp) {
    temp[--p] = arg_temp % 10 + '0';
    arg_temp /= 10;
  }
  if (arg < 0)  temp[--p] = '-';
  int digit = temp_len - 1 - p;

  int zero = 0;
  switch (info->fill_type) {
    case NONE:  case LEFT_ALIGN:  case RIGHT_ALIGN:
      if (info->precision == 0 && arg == 0) {
        zero = digit = 0;
      } else {
        zero = info->precision > digit ? info->precision - digit : 0;
      }
      break;
    case LEADING_ZERO:
      zero = info->width > digit ? info->width - digit : 0;
      break;
    default:  break;
  }
  p -= zero;
  digit += zero;
  memset(temp + p, '0', zero);

  int space = info->width - digit;
  space = space >= 0 ? space : 0;
  int count = 0;
  switch (info->fill_type) {
    case RIGHT_ALIGN:
      count += fill_c(out, ' ', space);
    case NONE:          
    case LEADING_ZERO:  
      count += fill_data(out, temp + p, digit);
      break;
    case LEFT_ALIGN:
      count += fill_data(out, temp + p, digit);
      count += fill_c(out, ' ', space);
      break;
    default:  break;
  }

  return count;
}

__attribute__((unused))
static int parse_str(ostream_t *out, info_t *info, va_list *args) {
  char *arg = va_arg(*args, char *);
  int len = strlen(arg);
  if (info->precision != -1) {
    len = len > info->precision ? info->precision : len;
  }

  int space = info->width - len;
  space = space >= 0 ? space : 0;
  int count = 0;
  switch (info->fill_type) {
    case RIGHT_ALIGN:
      count += fill_c(out, ' ', space);
    case NONE:          
      count += fill_data(out, arg, len);
      break;
    case LEFT_ALIGN:
      count += fill_data(out, arg, len);
      count += fill_c(out, ' ', space);
      break;
    default:  break;
  }

  return count;
}

// no buf version
int printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  info_t info;
  int count = 0;
  ostream_t os = {STDOUT, NULL, NULL};

  while (*fmt != '\0') {
    if (*fmt != '%') {
      putch(*fmt++);
      count++;
      continue;
    }

    if (*(fmt + 1) == '%') {
      putch('%');
      fmt += 2;
      count++;
      continue;
    }

    fmt++;
    fmt = parse_format(fmt, &info);
    count += info.parse(&os, &info, &args);
  }

  va_end(args);

  return count;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  info_t info;
  int count = 0;
  ostream_t os = {STR, out, NULL};

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
    count += info.parse(&os, &info, &ap);
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
  panic("Not implemented");
  // char buf[BUF_SIZE] = "";
  // int p = 0;

  // info_t info;
  // int count = 0;

  // char *end = out + n - 2;

  // while (*fmt != '\0') {
  //   if (out > end) {
  //     out = buf;
  //     end = buf + BUF_SIZE - 1;
  //   }

  //   if (*fmt != '%') {
  //     *out++ = *fmt++;
  //     count++;
  //     continue;
  //   }

  //   if (*(fmt + 1) == '%') {
  //     *out++ = '%';
  //     fmt += 2;
  //     count++;
  //     continue;
  //   }

  //   fmt++;
  //   fmt = parse_format(fmt, &info);
  //   char *new_out = info.parse(out, end, &info, &ap);
  //   count += new_out - out;
  //   out = new_out;
  // }
  // *out = '\0';
  // return count;
}

#endif
