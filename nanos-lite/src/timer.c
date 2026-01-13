#include <common.h>
#include <timer.h>

int tm_gettimeofday(struct timeval *tv, struct timezone *tz) {
	AM_TIMER_UPTIME_T time;
	ioe_read(AM_TIMER_UPTIME, &time);
	tv->tv_usec = time.us;
	tv->tv_sec = time.us / 1000000;
	return 0;
}