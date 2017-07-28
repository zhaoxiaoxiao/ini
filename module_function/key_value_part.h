
#ifndef __PROGRAM_FRAMEWORK_KEYVALUE_H__
#define __PROGRAM_FRAMEWORK_KEYVALUE_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef struct key_value{
	char *key_;
	char *value_;
	struct key_value *next_;
}KEY_VALUE_NODE;

KEY_VALUE_NODE* create_key_value_head(char* key,char* value,ngx_pool_t* mem);

KEY_VALUE_NODE* add_key_value_tail(KEY_VALUE_NODE* pre,char* key,char* value,ngx_pool_t* mem);

void add_keyvalue_after_head(KEY_VALUE_NODE* head,KEY_VALUE_NODE *node);

char* find_value_after_head(KEY_VALUE_NODE* head,char *key);

#ifdef __cplusplus
}
#endif
#endif

