#include <am.h>
#include <nemu.h>
#include "stdio.h"

static uint64_t start_time;

static uint64_t read_time() {
  return ((uint64_t) inl(RTC_ADDR)) | (((uint64_t) inl(RTC_ADDR + 4)) << 32);
}

void __am_timer_init() {
  start_time = read_time();
  printf("start: %d+", (int)(start_time >> 32));
  printf("%d\n", (int)start_time); 
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  uptime->us = read_time() - start_time;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
