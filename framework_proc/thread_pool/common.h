
#ifndef __PROGRAM_FRAMEWORK_COMMON_H__
#define __PROGRAM_FRAMEWORK_COMMON_H__

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

#if 1
#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: DEBUG " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
									
#endif