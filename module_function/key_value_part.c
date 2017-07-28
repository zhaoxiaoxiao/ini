

#include "common.h"
#include "frame_tool.h"
#include "memory_pool.h"
#include "key_value_part.h"


void add_keyvalue_after_head(KEY_VALUE_NODE* head,KEY_VALUE_NODE *node)
{
	KEY_VALUE_NODE *tail = head;
	if(head == NULL)
		head = node;
	else{
		while(tail->next_)
		{
			tail = tail->next_;
		}
		tail->next_ = node;
	}
}

char* find_value_after_head(KEY_VALUE_NODE* head,char *key)
{
	int ret = 0,str_len = 0;
	KEY_VALUE_NODE *find = head;
	if(head == NULL)
		return NULL;
	else{
		str_len = frame_strlen(key);
		while(find)
		{
			if(frame_strlen(find->key_) == str_len)
			{
				ret = memcmp(find->key_,key,str_len);
				if(ret == 0)
				{
					return find->value_;
				}
			}
			find = find->next_;
		}
		return NULL;
	}
}


