
#include "common.h"
#include "memory_pool.h"
#include "key_value_part.h"
#include "es_obj_info.h"



void add_objinfo_after_head(OBJ_INFO *head,OBJ_INFO *node)
{
	OBJ_INFO *tail = NULL;
	if(head == NULL)
		head = node;
	else{
		tail = head;
		while(tail->next_)
			tail = tail->next_;
		tail->next_ = node;
	}
}

void add_objinfo_after_tail(OBJ_INFO **tail,OBJ_INFO *node)
{
	if(*tail)
	{
		(*tail)->next_ = node;
		*tail = node;
	}
}

