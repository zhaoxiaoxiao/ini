
#ifndef __PROGRAM_FRAMEWORK_ESHTTPAPI_H__
#define __PROGRAM_FRAMEWORK_ESHTTPAPI_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef struct es_respond ES_RESPOND;

typedef void (*RESPOND_CALLBACL)(ES_RESPOND *ret);

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

typedef struct es_request{
	const char *path_str;
	const char *query_str;
	const char *body_str;
	VERB_METHOD verb;
}ES_REQUEST;

struct es_respond{
	int sta_;//0 is init,1 is send,2 is recv,3is call back
	int res_sta_;
	int num_;
	OBJ_INFO *head;
	OBJ_INFO *tail;
	ngx_pool_t *mem;

	ES_REQUEST *req_;
	char *req_buf;
	RESPOND_CALLBACL call_;
};

int es_server_init(const char *ip_str,unsigned short port,PROTOCOL_TYPE type);

int es_query_asynchronous(ES_REQUEST *req,RESPOND_CALLBACL call);

ES_RESPOND* es_query_block(ES_REQUEST *req);

void release_es_respond();

void es_server_destroy();

#ifdef __cplusplus
}
#endif
#endif

