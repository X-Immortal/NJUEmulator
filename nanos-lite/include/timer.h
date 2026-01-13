#ifndef __TIMER_H__
#define __TIMER_H__

#include <common.h>
#include <sys/time.h>

int tm_gettimeofday(struct timeval *tv, struct timezone *tz);

#endif