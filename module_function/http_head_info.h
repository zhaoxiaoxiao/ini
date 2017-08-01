
#ifndef __PROGRAM_FRAMEWORK_HTTPHEADINFO_H__
#define __PROGRAM_FRAMEWORK_HTTPHEADINFO_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef struct http_head_info{
	char *ver;
	char *note;
	int ret;
	int body_len;
	KEY_VALUE_NODE *head_;
}HTTP_HEAD_INFO;

#ifdef __cplusplus
}
#endif
#endif

