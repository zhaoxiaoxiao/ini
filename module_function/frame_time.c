
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

char* get_nowdate_str(char *str,int len)
{
	
}


void gettimespace()
{
	struct timespec time1 = {0, 0};

	clock_gettime(CLOCK_REALTIME, &time1);//1970-1-1 0:0:0
	clock_gettime(CLOCK_MONOTONIC, &time1);//machine start time
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);//process start time
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time1);//thread start time
	
}

void gettimeval()
{
	struct timeval tval = {0};

	gettimeofday(&tval,NULL);
}

