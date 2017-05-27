
#ifndef __PROGRAM_FRAMEWORK_TIME_H__
#define __PROGRAM_FRAMEWORK_TIME_H__


#ifdef __cplusplus
extern "C" {
#endif

unsigned long get_nowtime_second();

char* get_nowdate_str(char *str,int len);
#ifdef __cplusplus
}
#endif
#endif

