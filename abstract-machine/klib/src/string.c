#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

#define byte unsigned char
typedef unsigned long long int op_t;
#define OPSIZ (sizeof(op_t))
#define OP_T_THRES 8

size_t strlen(const char *s) {
  size_t len = 0;
  for (const char *p = s; *p != '\0'; len++, p++);
  return len;
}

char *strcpy(char *dst, const char *src) {
  char *p_dest = dst;
  const char *p_src = src;
  while ((*p_dest++ = *p_src++) != '\0');
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  char *p_dest = dst;
  const char *p_src = src;
  for (size_t i = 0; i < n && (*p_dest++ = *p_src++) != '\0'; i++);
  return dst;
}

char *strcat(char *dst, const char *src) {
  char *p_dest = dst;
  while (*p_dest != '\0') p_dest++;
  strcpy(p_dest, src);
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  for (; *s1 != '\0' && *s1 == *s2; s1++, s2++);
  return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  size_t i;
  for (i = 0; i < n && *s1 && *s1 == *s2; i++, s1++, s2++);
  if (i == n) return 0;
  return *s1 - *s2;
}

void *memset(void *s, int c, size_t n) {
  uintptr_t dest = (uintptr_t) s;

  if (n >= OP_T_THRES) {
    // get a word to set
    op_t cccc = (byte) c;
    cccc |= cccc << 8;
    cccc |= cccc << 16;
    if (OPSIZ > 4) {
      cccc |= (cccc << 16) << 16;
    }

    // set by byte until aligned
    while (dest % OPSIZ != 0) {
      *(byte *) dest = c;
      ++dest;
      --n;
    }

    // set by op_t while n is enough
    int xlen = n / OPSIZ;
    while (xlen > 0) {
      ((op_t *) dest)[0] = cccc;
      dest += OPSIZ;
      --xlen;
    }
    n %= OPSIZ;
  }

  // set by byte
  while (n > 0) {
    *(byte *) dest = c;
    ++dest;
    --n;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  if (dst == src) return dst;

  unsigned char *p_dest = dst;
  const unsigned char *p_src = src;
  if (p_dest > p_src && p_dest < p_src + n) {
    size_t i = n - 1;
    while (true) {
      *(p_dest + i) = *(p_src + i);
      if (i == 0) break;
      i--;
    }
  } else {
    memcpy(dst, src, n);
  }

  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  uintptr_t dest = (uintptr_t) out;
  uintptr_t src = (uintptr_t) in;

  if (n >= OP_T_THRES) {
    // copy by byte until dest is aligned
    while (dest % OPSIZ != 0) {
      *(byte *) dest++ = *(byte *) src++;
      --n;
    }

    if (src % OPSIZ == 0) {
      // aligned copy
      while (n > OPSIZ) {
        *(op_t *) dest = *(op_t *) src;
        dest += OPSIZ;
        src += OPSIZ;
        n -= OPSIZ;
      }
    } else {
      // unaligned copy

      // |...xxxxx|xxx.....|
      //      high low 
      // op_t high = 0;
      // int low_len = src % OPSIZ;
      // int high_len = OPSIZ - low_len;
      // for (int i = 0; i < high_len; ++i) {
      //   high |= *(byte *) src++;
      //   high <<= 8;
      // }

      // while (n > OPSIZ + high_len) {
      //   op_t data = *(op_t *) src;
      //   op_t low = data >> (high_len << 3);
      //   *(op_t *) dest = (high | low);
      //   high = data << (low_len << 3);
      //   dest += OPSIZ;
      //   src += OPSIZ;
      //   n -= OPSIZ;
      // }
    }
  }

  // copy by byte
  while (n > 0) {
    *(byte *) dest++ = *(byte *) src++;
    --n;
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = s1, *p2 = s2;
  size_t i;
  for (i = 0; i < n && *p1 == *p2; i++, p1++, p2++);
  if (i == n) return 0;
  return *p1 - *p2;
}

#endif
