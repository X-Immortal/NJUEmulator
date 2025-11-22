#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

typedef enum {
  LEFT_ALIGN, RIGHT_ALIGN, LEADING_ZERO, NONE,
} fill_t;

struct info_t;

typedef char *(*parse_t)(char *out, struct info_t *info, va_list *args);

typedef struct info_t {
  parse_t parse;
  fill_t fill_type;
  int width;
  int precision;
} info_t;

static char *parse_int(char *out, info_t *info, va_list *args);
static char *parse_str(char *out, info_t *info, va_list *args);

struct {
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
    if (info->fill_type == LEADING_ZERO) {
      info->fill_type = RIGHT_ALIGN;
    }
    fmt++;
    int precision = 0;
    while (*fmt >= '0' && *fmt <= '9') {
      precision = precision * 10 + *fmt++ - '0';
    }
    info->precision = precision;
  }

  if (info->fill_type == LEADING_ZERO) {
    info->width = -1;
  }

  for (int i = 0; i < LENGTH(placeholders); i++) {
    int ph_len = strlen(placeholders[i].ph);
    if (strncmp(fmt, placeholders[i].ph, ph_len) == 0) {
      info->parse = placeholders[i].parse;
      fmt += ph_len;
      break;
    }
  }
  return fmt;
}

__attribute__((unused))
static char *parse_int(char *out, info_t *info, va_list *args) {
  int arg = va_arg(*args, int);
  int arg_temp = arg;
  char temp[12] = "";
  int p = 11;
  while (arg_temp) {
    temp[--p] = arg_temp % 10 + '0';
    arg_temp /= 10;
  }
  if (arg < 0)  temp[--p] = '-';
  int digit = 11 - p;

  int zero = 0;
  int width = 0;
  switch (info->fill_type) {
    case NONE:          
      if (!(info->precision == 0 && arg == 0)) {
        zero = info->precision - digit;
        memset(out, '0', zero);
        out += zero;
      }
      strcpy(out, temp + p);
      out += digit;
      break;
    case LEADING_ZERO:  
      zero = info->precision - digit;
      memset(out, '0', zero);
      out += zero;
      strcpy(out, temp + p);
      out += digit;
      break;
    case LEFT_ALIGN:
      zero = 0;
      width = 0;
      if (info->precision > digit) {
        zero = info->precision - digit;
        memset(out, '0', zero);
        out += zero;
        width += zero;
      }
      if (!(info->precision == 0 && arg == 0)) {
        strcpy(out, temp + p);
        out += digit;
        width += digit;
      }
      memset(out, ' ', info->width - width);
      out += info->width - width;
      break;
    case RIGHT_ALIGN:
      zero = 0;
      width = 0;
      if (info->precision > digit) {
        zero = info->precision - digit;
        width += zero;
      }
      if (!(info->precision == 0 && arg == 0)) {
        width += digit;
      }
      memset(out, ' ', info->width - width);
      out += info->width - width;
      memset(out, '0', zero);
      out += zero;
      if (width > 0){
        strcpy(out, temp + p);
        out += digit;
      }  
      break;
    default:  break;
  }

  return out;
}

__attribute__((unused))
static char *parse_str(char *out, info_t *info, va_list *args) {
  return NULL;
}

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  info_t info;
  int count = 0;

  while (*fmt != '\0') {
    if (*fmt != '%') {
      *out++ = *fmt++;
      continue;
    }

    if (*(fmt + 1) == '%') {
      *out++ = '%';
      fmt += 2;
      continue;
    }

    fmt++;
    fmt = parse_format(fmt, &info);
    out = info.parse(out, &info, &ap);
    count++;
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
