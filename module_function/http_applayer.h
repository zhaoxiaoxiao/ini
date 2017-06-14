
#ifndef __PROGRAM_FRAMEWORK_HTTPAPPL_H__
#define __PROGRAM_FRAMEWORK_HTTPAPPL_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef struct make_http_message{
	char *h_head;
	char *h_buf;

	char *path;
	char *body;
	char *ip_str;
	
	int h_buf_len;
	unsigned short port;
}MAKE_HTTP_MESSAGE;

int make_es_http_message(MAKE_HTTP_MESSAGE *param);

#ifdef __cplusplus
}
#endif
#endif

