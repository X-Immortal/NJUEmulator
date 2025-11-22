#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

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
  unsigned char value = c;
  unsigned char *p = s; 
  for (size_t i = 0; i < n; i++)  *p++ = value;
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  if (dst == src) return dst;

  unsigned char *p_dest = dst;
  const unsigned char *p_src = src;
  if (p_dest > p_src && p_dest < p_src + n) {
    for (size_t i = n - 1; i >= 0; i--) *(p_dest + i) = *(p_src + i);
  } else {
    memcpy(dst, src, n);
  }

  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  unsigned char *dest = out;
  const unsigned char *src = in;
  for (size_t i = 0; i < n; i++) *dest++ = *src++;
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
