
#ifndef __PROGRAM_FRAMEWORK_ESOBJINFO_H__
#define __PROGRAM_FRAMEWORK_ESOBJINFO_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef struct obj_info{
	char *index;
	char *type;
	char *id_;
	KEY_VALUE_NODE *head_;
	struct obj_info *next_;
}OBJ_INFO;

OBJ_INFO* create_obj_info_head(char *id,ngx_pool_t* mem);

void add_obj_info_tail(OBJ_INFO *pre,char *id,ngx_pool_t* mem);

void add_objinfo_after_head(OBJ_INFO **head,OBJ_INFO *node);

void add_objinfo_after_tail(OBJ_INFO **tail,OBJ_INFO *node);

#ifdef __cplusplus
}
#endif
#endif

