
#include <time.h>
#include <sys/time.h>

#include "common.h"
#include "frame_time.h"

unsigned long get_nowtime_second()
{
	unsigned long second = 0;
	time_t sys_time_t;
	time(&sys_time_t);
	second = sys_time_t;
	return second;
}

