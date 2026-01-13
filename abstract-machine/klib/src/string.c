#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

#define byte unsigned char
typedef unsigned long int op_t;
#define OPSIZ (sizeof(op_t))
#define OP_T_THRES 8

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define MERGE(l_word, l_off, h_word, h_off) (((l_word) >> (l_off)) | ((h_word) << (h_off)))
#else
#define MERGE(l_word, l_off, h_word, h_off) (((l_word) << (l_off)) | ((h_word) >> (h_off)))
#endif

static inline bool has_zero(op_t word) {
  if (OPSIZ == 4) {
      unsigned long mask = 0x80808080UL;
      return (word - 0x01010101UL) & ~word & mask;
  } else if (OPSIZ == 8) {
      unsigned long long mask = 0x8080808080808080ULL;
      return (word - 0x0101010101010101ULL) & ~word & mask;
  } else {
    panic("unsupported word length");
  }
}

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
  // compare by byte until s1 is aligned
  uintptr_t off = -(uintptr_t) s1 % OPSIZ;
  while (off > 0) {
    if (*s1 == '\0' || *s1 != *s2) {
      return *s1 - *s2;
    }
    --off;
  }

  uintptr_t p1 = (uintptr_t) s1;
  uintptr_t p2 = (uintptr_t) s2;
  if (p2 % OPSIZ == 0) {
    // aligned compare
    op_t w1, w2;
    while (true) {
      w1 = *(op_t *) p1;
      w2 = *(op_t *) p2;
      if (has_zero(w1) || w1 != w2) {
        break;
      }
      p1 += OPSIZ;
      p2 += OPSIZ;
    }
  } else {
    // unaligned compare

    // |...xxxxx|xxx.....|
    //    l_word  r_word
    int l_off = p2 % OPSIZ;
    int h_off = OPSIZ - l_off;
    p2 -= l_off;
    op_t l_word, h_word;
    l_word = *(op_t *) p2;
    p2 += OPSIZ;
    op_t w1, w2;
    while (true) {
      w1 = *(op_t *) p1;
      h_word = *(op_t *) p2;
      w2 = MERGE(l_word, l_off << 3, h_word, h_off << 3);
      if (has_zero(w1) || w1 != w2) {
        p2 -= h_off;
        break;
      }
      p1 += OPSIZ;
      p2 += OPSIZ;
      l_word = h_word;
    }
  }

  s1 = (const char *) p1;
  s2 = (const char *) p2;
  for (; *s1 && *s1 == *s2; ++s1, ++s2);
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

  uintptr_t _dest = (uintptr_t) dst;
  uintptr_t _src = (uintptr_t) src;

  if (_dest < _src || _dest - _src >= n) {
    // copy from the beginning to the end
    memcpy(dst, src, n);
  } else {
    // copy from the end to the beginning
    _dest += n;
    _src += n;

    if (n >= OP_T_THRES) {
      // copy by byte until dest is aligned
      while (_dest % OPSIZ != 0) {
        *(byte *) --_dest = *(byte *) --_src;
        --n;
      }

      if (_src % OPSIZ == 0) {
        // aligned copy
        while (n > OPSIZ) {
          _dest -= OPSIZ;
          _src -= OPSIZ;
          *(op_t *) _dest = *(op_t *) _src;
          n -= OPSIZ;
        }
      } else {
        // unaligned copy

        // |...xxxxx|xxx.....|
        //    l_word  r_word
        int l_off = _src % OPSIZ;
        int h_off = OPSIZ - l_off;
        _src -= l_off;
        op_t l_word, h_word;
        h_word = *(op_t *) _src;
        while (n > OPSIZ + l_off) {
          _dest -= OPSIZ;
          _src -= OPSIZ;
          l_word = *(op_t *) _src;
          *(op_t *) _dest = MERGE(l_word, l_off << 3, h_word, h_off << 3);
          n -= OPSIZ;
          h_word = l_word;
        }
        _src += l_off;
      }
    }

    // copy by byte
    while (n > 0) {
      *(byte *) --_dest = *(byte *) --_src;
      --n;
    }
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
    printf("called memcpy\n");

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
      //    l_word  r_word
      int l_off = src % OPSIZ;
      int h_off = OPSIZ - l_off;
      src -= l_off;
      op_t l_word, h_word;
      l_word = *(op_t *) src;
      src += OPSIZ;
      while (n > OPSIZ + h_off) {
        h_word = *(op_t *) src;
        *(op_t *) dest = MERGE(l_word, l_off << 3, h_word, h_off << 3);
        dest += OPSIZ;
        src += OPSIZ;
        n -= OPSIZ;
        l_word = h_word;
      }
      src -= h_off;
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
