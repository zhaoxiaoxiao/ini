
#ifndef __PROGRAM_FRAMEWORK_ESHTTPAPI_H__
#define __PROGRAM_FRAMEWORK_ESHTTPAPI_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef enum protocol_type{
	HTTP,
	HTTPS,
}PROTOCOL_TYPE;

typedef enum verb_method{
	GET_METHOD,
	POST_METHOD,
	PUT_METHOD,
	HEAD_METHOD,
	DELETE_METHOD,
}VERB_METHOD;

int es_server_init(const char *ip_str,unsigned short port,PROTOCOL_TYPE type);

void es_server_query(VERB_METHOD verb,const char *path_str,const char *query_str,const char *body_str);

void es_server_destroy();

#ifdef __cplusplus
}
#endif
#endif

