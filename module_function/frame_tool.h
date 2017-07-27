

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

char* framestr_frist_constchar(char *str,char c);

char* framestr_end_digital_char(char *dig_str);

char* framestr_last_constchar(char *str,char c);

char* framestr_first_conststr(char *str,char *tar);

char *current_user_name(char *name,int len);


size_t get_system_pagesize();

#ifdef __cplusplus
}
#endif
#endif
