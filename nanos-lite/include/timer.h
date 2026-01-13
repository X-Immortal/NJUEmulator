#ifndef __TIMER_H__
#define __TIMER_H__

#include <sys/time.h>

int tm_gettimeofday(struct timeval *tv, struct timezone *tz);

#endif