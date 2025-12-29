#include <am.h>
#include <nemu.h>
#include "stdio.h"

#define RTC_HIGH_ADDR (RTC_ADDR + 4)

static uint64_t start_time;

static uint64_t read_time() {
  while (true) {
    uint32_t high1 = inl(RTC_HIGH_ADDR);
    uint32_t low   = inl(RTC_ADDR);
    uint32_t high2 = inl(RTC_HIGH_ADDR);
    if (high1 == high2) {
      return (((uint64_t) high1) << 32) | ((uint64_t) low);
    }
  }
}

void __am_timer_init() {
  start_time = read_time();
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  uint64_t time = read_time();
  uptime->us = time - start_time;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
