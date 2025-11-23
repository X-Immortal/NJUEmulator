#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

#define DEFAULT_BUF_SIZE 4096

// static size_t buf_size = DEFAULT_BUF_SIZE;

typedef enum {
  LEFT_ALIGN, RIGHT_ALIGN, LEADING_ZERO, NONE,
} fill_t;

struct info_t;

typedef char *(*parse_t)(char *out, char *end, struct info_t *info, va_list *args);

typedef struct info_t {
  parse_t parse;
  fill_t fill_type;
  int width;
  int precision;
} info_t;

static char *parse_int(char *out, char *end, info_t *info, va_list *args);
static char *parse_str(char *out, char *end, info_t *info, va_list *args);

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

static char *fill_space(char *out, char *end, int n) {
  if (end != NULL && end - out < n) {
    memset(out, ' ', end - out);
    return out = end;
  }
  memset(out, ' ', n);
  return out += n;
}
 
static char *fill_data(char *out, char *end, const char *data, int n) {
  if (end != NULL && end - out < n) {
    strncpy(out, data, end - out);
    return out = end;
  }
  strncpy(out, data, n + 1);
  return out += n;
}

__attribute__((unused))
static char *parse_int(char *out, char *end, info_t *info, va_list *args) {
  int arg = va_arg(*args, int);
  int arg_temp = arg;

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
  switch (info->fill_type) {
    case RIGHT_ALIGN:
      out = fill_space(out, end, space);
    case NONE:          
    case LEADING_ZERO:  
      out = fill_data(out, end, temp + p, digit);
      break;
    case LEFT_ALIGN:
      out = fill_data(out, end, temp + p, digit);
      out = fill_space(out, end, space);
      break;
    default:  break;
  }

  return out;
}

__attribute__((unused))
static char *parse_str(char *out, char *end, info_t *info, va_list *args) {
  char *arg = va_arg(*args, char *);
  int len = strlen(arg);
  if (info->precision != -1) {
    len = len > info->precision ? info->precision : len;
  }

  int space = info->width - len;
  space = space >= 0 ? space : 0;
  switch (info->fill_type) {
    case RIGHT_ALIGN:
      out = fill_space(out, end, space);
    case NONE:          
      out = fill_data(out, end, arg, len);
      break;
    case LEFT_ALIGN:
      out = fill_data(out, end, arg, len);
      out = fill_space(out, end, space);
      break;
    default:  break;
  }

  return out;
}

int printf(const char *fmt, ...) {
  // char buf[buf_size];
  // va_list args;
  // va_start(args, fmt);
  
  // va_end(args);
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  info_t info;
  int count = 0;

  while (*fmt != '\0') {
    if (*fmt != '%') {
      *out++ = *fmt++;
      count++;
      continue;
    }

    if (*(fmt + 1) == '%') {
      *out++ = '%';
      fmt += 2;
      count++;
      continue;
    }

    fmt++;
    fmt = parse_format(fmt, &info);
    char *end = info.parse(out, NULL, &info, &ap);
    count += end - out;
    out = end;
  }
  *out = '\0';
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
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
