
#ifndef __PROGRAM_FRAMEWORK_ESEARCHRESULT_H__
#define __PROGRAM_FRAMEWORK_ESEARCHRESULT_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef struct es_search_result{
	int total;
	OBJ_INFO *head;
	OBJ_INFO *tail;
}ES_SEARCH_RESULT;


#ifdef __cplusplus
}
#endif
#endif

