

#ifndef __PROGRAM_FRAMEWORK_TOOL_H__
#define __PROGRAM_FRAMEWORK_TOOL_H__


#ifdef __cplusplus
extern "C" {
#endif

int lock_mutex_file();

void release_mutex_file();

int cpu_isbig_endian();

char *frame_end_charstr(char *buff);

int frame_strlen(const char *str);

int framestr_isall_digital(const char *str);

char* framestr_frist_constchar(char *str,const char c);

char* framestr_last_constchar(char *str,const char c);

char* framestr_first_conststr(char *str,const char *tar);

#ifdef __cplusplus
}
#endif
#endif
