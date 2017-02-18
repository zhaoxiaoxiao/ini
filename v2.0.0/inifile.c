
/*
*   程序设计为读写操作ini文件使用，设计思想：
*   ini文件节点构成一个链表，链表中每个节点下有键值链表,如果文件刚刚开始为注释，则为一个节点。
*   每对键值构成键值链表中一个节点，
*   其实读写ini文件就是对这些节点的操作
*   ini文件中空白符与制表符自动忽略
*
*
*	注意下面几个常量的定义，长度不能越界
*	每个节点或者键值字符串最大长度不能大于 MAX_STRING_LEN
*	文件名长度不能大于MAX_FILENAME_LEN
*	配置文件中每一行长度不能大于 MAX_FILLINE_LEN
*	传递参数字符数组必须以'\0'结尾
*/

#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h> 

//#include <sys/io.h>
#include <fcntl.h>
#include <errno.h>

#include "inifile_t.h"

#define CONTENT_NEELINE_SIGN			10
#define CONTENT_SPACE_SIGN				32
#define CONTENT_TAB_SIGN				9

#define CONTENT_LEFT_BRACKET			91//[
#define CONTENT_RIGHT_BRACKET			93///]
#define CONTENT_COMMA_SIGN				59//;
#define CONTENT_EQUALITY_SIGN			61//=

#define INIFILE_MAX_CONTENT_LINELEN		1024
#define INIFILE_KEYVALUE_ARRAY_LEN		(MAX_INIFILE_SECTION_NUM * MAX_KEYVALUE_UNDER_SECTION)

#define INIFILE_SPACE_CHAR_LEN			1
#define INIFILE_SYSTEM_CMD_LEN			512
#define INIFILE_FILEALL_NAME_LEN		128

#define IS_SHOWDEBUG_MESSAGE			1

#define STRUCT_NODE_IN_USED				1
#define STRUCT_NODE_UN_USED				0

#define INI_FILE_MEMORY_POOL_INTI		0
#define INI_FILE_MEMORY_POOL_BACK		1

#if IS_SHOWDEBUG_MESSAGE
#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)

typedef enum ini_file_line_type
{
	LINE_SECTION,
	LINE_KEYVALUE,
	LINE_NOTE,
	LINE_BLANK,
	LINE_ERROR,
}INI_FILE_LINE_TYPE;

typedef struct memory_pool{
	int flag;
	char pool[INIFILE_SIZE_SPACE];
	char back_pool[INIFILE_SIZE_SPACE];
	char *current;
	int curr_len;
	pthread_mutex_t pool_lock;
}MEMORY_POOL;

typedef struct ini_keyvalue{
	int flag;
	const char *key;
	const char *value;
	struct ini_keyvalue *pre;
	struct ini_keyvalue *last;
}INI_KEYVALUE;

typedef struct ini_keyvalue_array{
	INI_KEYVALUE keyval_array[INIFILE_KEYVALUE_ARRAY_LEN];
	pthread_mutex_t array_lock;
}INI_KEYVALUE_ARRAY;

typedef struct ini_section{
	int flag;
	const char *sec;
	INI_KEYVALUE *head;
	struct ini_section *pre;
	struct ini_section *last;
}INI_SETION;

typedef struct ini_section_array{
	INI_SETION section_array[MAX_INIFILE_SECTION_NUM];
	pthread_mutex_t array_lock;
}INI_SECTION_ARRAY;

typedef struct ini_file{
	int flag;
	const char *name;
	INI_SETION *head;
	pthread_mutex_t file_lock;
}INI_FILE;

typedef struct ini_file_array{
	INI_FILE ini_array[MAX_OPERATION_FILE_NUM];
	pthread_mutex_t array_lock;
}INI_FILE_ARRAY;

static MEMORY_POOL pool_mem = {0};
static INI_FILE_ARRAY ini_array = {0};
static INI_SECTION_ARRAY section_array = {0};
static INI_KEYVALUE_ARRAY keyval_array = {0};

static const char tem_suff_name[] = ".tmp";
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int str_int_len(const char *str)
{
	int len = 0;
	if(!str)
		return len;
	while(*str)
	{
		str++;
		len++;
	}
	return len;
}

char* str_frist_constchar(char *str,const char c)
{
	char *p = str;
	if(!str)
		return NULL;
	while(*p)
	{
		if(*p == c)
			return p;
		else
			p++;
	}
	return NULL;
}

char *find_frist_endchar(char *buff)
{
	char *p = buff;
	while(*p!=0)
	{
		p++;
	}
	return p;
}

void ini_file_memory_init()
{
	int i;
	INI_FILE *p_file = ini_array.ini_array;
	if(pool_mem.current == NULL)
	{
		pool_mem.current = pool_mem.pool;
		pool_mem.curr_len = 0;
		pthread_mutex_init(&pool_mem.pool_lock, NULL);
		pthread_mutex_init(&ini_array.array_lock, NULL); 
		pthread_mutex_init(&section_array.array_lock, NULL); 
		pthread_mutex_init(&keyval_array.array_lock, NULL); 
		for(i = 0;i < MAX_OPERATION_FILE_NUM;i++)
		{
			pthread_mutex_init(&p_file->file_lock, NULL);
			p_file++;
		}
	}
}

int ini_file_memory_adjust()
{
	int i,len,ret = 0;
	INI_FILE *p_ini_file = ini_array.ini_array;
	INI_SETION *p_section = NULL;
	INI_KEYVALUE *p_keyval = NULL;
	
	if(INI_FILE_MEMORY_POOL_INTI == pool_mem.flag)
	{
		pool_mem.current = pool_mem.back_pool;
		pool_mem.curr_len = 0;
		pool_mem.flag = INI_FILE_MEMORY_POOL_BACK;
	}else{
		pool_mem.current = pool_mem.pool;
		pool_mem.curr_len = 0;
		pool_mem.flag = INI_FILE_MEMORY_POOL_INTI;
	}
	
	for(i = 0;i < MAX_OPERATION_FILE_NUM;i++)
	{
		if(p_ini_file->flag == STRUCT_NODE_UN_USED)
		{
			p_ini_file++;
			continue;
		}else{
			if(p_ini_file->name)
			{
				len = str_int_len(p_ini_file->name);
				pool_mem.curr_len += len + INIFILE_SPACE_CHAR_LEN;
				if(pool_mem.curr_len >= INIFILE_SIZE_SPACE)
				{
					ret = INIFILE_MEMORY_POOL_ERROR;
					goto error_out;
				}
				memcpy(pool_mem.current,p_ini_file->name,len);
				p_ini_file->name = pool_mem.current;
				pool_mem.current += len + INIFILE_SPACE_CHAR_LEN;
			}
			p_section = p_ini_file->head;
			while(p_section)
			{
				if(p_section->sec)
				{
					len = str_int_len(p_section->sec);
					pool_mem.curr_len += len + INIFILE_SPACE_CHAR_LEN;
					if(pool_mem.curr_len >= INIFILE_SIZE_SPACE)
					{
						ret = INIFILE_MEMORY_POOL_ERROR;
						goto error_out;
					}
					memcpy(pool_mem.current,p_section->sec,len);
					p_section->sec = pool_mem.current;
					pool_mem.current += len + INIFILE_SPACE_CHAR_LEN;
				}
				p_keyval = p_section->head;
				while(p_keyval)
				{
					if(p_keyval->key)
					{
						len = str_int_len(p_keyval->key);
						pool_mem.curr_len += len + INIFILE_SPACE_CHAR_LEN;
						if(pool_mem.curr_len >= INIFILE_SIZE_SPACE)
						{
							ret = INIFILE_MEMORY_POOL_ERROR;
							goto error_out;
						}
						memcpy(pool_mem.current,p_keyval->key,len);
						p_keyval->key = pool_mem.current;
						pool_mem.current += len + INIFILE_SPACE_CHAR_LEN;
					}

					if(p_keyval->value)
					{
						len = str_int_len(p_keyval->value);
						pool_mem.curr_len += len + INIFILE_SPACE_CHAR_LEN;
						if(pool_mem.curr_len >= INIFILE_SIZE_SPACE)
						{
							ret = INIFILE_MEMORY_POOL_ERROR;
							goto error_out;
						}
						memcpy(pool_mem.current,p_keyval->value,len);
						p_keyval->value = pool_mem.current;
						pool_mem.current += len + INIFILE_SPACE_CHAR_LEN;
					}
					p_keyval = p_keyval->last;
				}
				p_section = p_section->last;
			}
		}
	}
	ret = 0;
error_out:
	return ret;
}

int storage_char_memory(const char **dest,const char *src,int len)
{
	int ret = 0;
	pthread_mutex_lock(&pool_mem.pool_lock);
	pool_mem.curr_len += len + INIFILE_SPACE_CHAR_LEN;
	if(pool_mem.curr_len >= INIFILE_SIZE_SPACE)
	{
		ret = ini_file_memory_adjust();
		if(ret < 0)
			goto error_out;
		pool_mem.curr_len += len + INIFILE_SPACE_CHAR_LEN;
		if(pool_mem.curr_len >= INIFILE_SIZE_SPACE)
		{
			ret= INIFILE_MEMORY_POOL_OUT;
			goto error_out;
		}
	}
	memcpy(pool_mem.current,src,len);
	*dest = pool_mem.current;
	pool_mem.current += len + INIFILE_SPACE_CHAR_LEN;

error_out:	
	pthread_mutex_unlock(&pool_mem.pool_lock);
	return ret;
}

int get_index_ini_file()
{
	int i = 0;
	INI_FILE *p_ini_file = ini_array.ini_array;
	pthread_mutex_lock(&ini_array.array_lock);
	for(i = 0;i < MAX_OPERATION_FILE_NUM;i++)
	{
		if(p_ini_file->flag == STRUCT_NODE_UN_USED)
		{
			p_ini_file->flag = STRUCT_NODE_IN_USED;
			break;
		}
		p_ini_file++;
	}
	if(i >= MAX_OPERATION_FILE_NUM)
		i = INIFILE_NUM_OUT_ARRAY;
	pthread_mutex_unlock(&ini_array.array_lock);
	return i;
}

int get_index_ini_section()
{
	int i = 0;
	INI_SETION *p_section = section_array.section_array;
	pthread_mutex_lock(&section_array.array_lock);
	for(i = 0;i < MAX_INIFILE_SECTION_NUM;i++)
	{
		if(p_section->flag == STRUCT_NODE_UN_USED)
		{
			p_section->flag = STRUCT_NODE_IN_USED;
			break;
		}
		p_section++;
	}
	if(i >= MAX_INIFILE_SECTION_NUM)
		i = INIFILE_NUM_OUT_ARRAY;
	pthread_mutex_unlock(&section_array.array_lock);
	return i;
}

int get_index_ini_keyvalue()
{
	int i = 0;
	INI_KEYVALUE *p_key = keyval_array.keyval_array;
	pthread_mutex_lock(&keyval_array.array_lock);
	for(i = 0;i < INIFILE_KEYVALUE_ARRAY_LEN;i++)
	{
		if(p_key->flag == STRUCT_NODE_UN_USED)
		{
			p_key->flag = STRUCT_NODE_IN_USED;
			break;
		}
		p_key++;
	}
	if(i >= INIFILE_KEYVALUE_ARRAY_LEN)
		i = INIFILE_NUM_OUT_ARRAY;
	pthread_mutex_unlock(&keyval_array.array_lock);
	return i;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INI_FILE_LINE_TYPE judge_ini_file_linetype(char *line)
{
	char *pp = line,*p_brack = NULL,*q_brack = NULL,*p_equ = NULL,*p_comma = NULL;

	while(*pp == CONTENT_SPACE_SIGN || *pp == CONTENT_TAB_SIGN)
		pp++;
	
	if(*pp == CONTENT_NEELINE_SIGN)
		return LINE_BLANK;
	
	p_comma = str_frist_constchar(line,CONTENT_COMMA_SIGN);
	p_brack = str_frist_constchar(line,CONTENT_LEFT_BRACKET);
	p_equ = str_frist_constchar(line,CONTENT_EQUALITY_SIGN);

	if(p_comma)
	{
		if(p_brack && p_equ)
		{
			if(p_comma < p_brack && p_comma < p_equ)
			{
				while(pp < p_comma)
				{
					if(*pp == CONTENT_SPACE_SIGN || *pp == CONTENT_TAB_SIGN)
						pp++;
					else
						return LINE_ERROR;
				}
				return LINE_NOTE;
			}else if(p_comma > p_brack && p_comma > p_equ)
			{
				return LINE_ERROR;
			}else if(p_comma > p_brack)
			{
				q_brack = str_frist_constchar(line,CONTENT_RIGHT_BRACKET);
				if(q_brack && q_brack < p_comma)
					return LINE_SECTION;
				else
					return LINE_ERROR;
			}else{
				return LINE_KEYVALUE;
			}
		}else if(p_brack){
			if(p_comma <  p_brack)
			{
				while(pp < p_comma)
				{
					if(*pp == CONTENT_SPACE_SIGN || *pp == CONTENT_TAB_SIGN)
						pp++;
					else
						return LINE_ERROR;
				}
				return LINE_NOTE;
			}else{
				q_brack = str_frist_constchar(line,CONTENT_RIGHT_BRACKET);
				if(q_brack && q_brack < p_comma)
					return LINE_SECTION;
				else
					return LINE_ERROR;
			}
		}else if(p_equ)
		{
			if(p_comma < p_equ)
			{
				while(pp < p_comma)
				{
					if(*pp == CONTENT_SPACE_SIGN || *pp == CONTENT_TAB_SIGN)
						pp++;
					else
						return LINE_ERROR;
				}
				return LINE_NOTE;
			}
			else
				return LINE_KEYVALUE;
		}else{
			while(pp < p_comma)
			{
				if(*pp == CONTENT_SPACE_SIGN || *pp == CONTENT_TAB_SIGN)
					pp++;
				else
					return LINE_ERROR;
			}
			return LINE_NOTE;
		}
	}else{
		if(p_brack && p_equ)
			return LINE_ERROR;
		else if(p_brack)
		{
			q_brack = str_frist_constchar(line,CONTENT_RIGHT_BRACKET);
			if(q_brack && q_brack > p_brack)
				return LINE_SECTION;
			else
				return LINE_ERROR;
		}else if (p_equ)
			return LINE_KEYVALUE;
		else
			return LINE_ERROR;
	}
}

INI_KEYVALUE *find_keyvalue_unsect(INI_KEYVALUE *head,const char *key,int len)
{
	int ret = 0,c_len = 0;
	INI_KEYVALUE *p_key = head;

	if(head == NULL || key == NULL)
		return NULL;

	if(len <= 0)
		len = str_int_len(key);
	
	while(p_key)
	{
		c_len = str_int_len(p_key->key);
		if(c_len == len)
		{
			ret = memcmp((const void *)(p_key->key),(const void *)(p_key->value),len);
			if(ret == 0)
				break;
		}
		p_key = p_key->last;
	}

	return p_key;
}


int init_line_key_val(char *line)
{
	int index = 0,len = 0,ret = 0;
	char *p = line,*p_equ = NULL,*pp = NULL,*p_comma = NULL;
	INI_KEYVALUE *p_key = keyval_array.keyval_array;
	
	p_equ = str_frist_constchar(line,CONTENT_EQUALITY_SIGN);
	p_comma = str_frist_constchar(line,CONTENT_COMMA_SIGN);

	if(p_comma && p_comma < p_equ)
		return INIFILE_FORMAT_ERROR;

	if(p_equ && p < p_equ)
	{
		index = get_index_ini_keyvalue();
		if(index < 0)
			return index;

		p_key += index;		
		while(*p == CONTENT_TAB_SIGN || *p == CONTENT_SPACE_SIGN)
			p++;
		pp = p_equ - INIFILE_SPACE_CHAR_LEN;
		while(*pp == CONTENT_TAB_SIGN || *pp == CONTENT_SPACE_SIGN)
			pp--;
		if(pp <= p)
		{
			ret = INIFILE_FORMAT_ERROR;
			goto error_out;
		}
		len = pp - p + INIFILE_SPACE_CHAR_LEN;
		ret = storage_char_memory(&(p_key->key),p,len);
		if(ret < 0)
			goto error_out;

		if(p_comma)
		{
			pp = p_comma;
		}else{		
			pp = find_frist_endchar(p_equ);
		}
		pp--;
		while(*pp == CONTENT_NEELINE_SIGN || *pp == CONTENT_TAB_SIGN || *pp == CONTENT_SPACE_SIGN)
			pp--;
		p = p_equ + INIFILE_SPACE_CHAR_LEN;
		while(*p == CONTENT_TAB_SIGN || *p == CONTENT_SPACE_SIGN)
			p++;
		if(pp <= p)
		{
			ret = INIFILE_FORMAT_ERROR;
			goto error_out;
		}
		len = pp - p + INIFILE_SPACE_CHAR_LEN;
		ret = storage_char_memory(&(p_key->value),p,len);
		if(ret < 0)
			goto error_out;
		return index;	
	}
	return INIFILE_FORMAT_ERROR;
error_out:
	p_key->flag = STRUCT_NODE_UN_USED;
	return ret;
}

INI_SETION *find_inifile_section(INI_SETION * head,const char *name,int len)
{
	int ret = 0,c_len = 0;
	INI_SETION *p_section = head;
	if(head == NULL || name == NULL)
		return NULL;

	if(len <= 0)
		len = str_int_len(name);
	while(p_section)
	{
		c_len = str_int_len(p_section->sec);
		if(len == c_len)
		{
			ret = memcmp((const void *)(p_section->sec),(const void *)name,len);
			if(ret == 0)
				break;
		}
		p_section = p_section->last;
	}
	return p_section;
}

int init_line_section(char *line)
{
	int index = 0,len = 0,ret = 0;
	char *pp = line,*p_brack = NULL,*q_brack = NULL;
	INI_SETION *p_section = section_array.section_array;
	
	p_brack = str_frist_constchar(line,CONTENT_LEFT_BRACKET);
	q_brack = str_frist_constchar(line,CONTENT_RIGHT_BRACKET);

	index = get_index_ini_section();
	if(index < 0)
		return index;
	p_section += index;

	if(p_brack && q_brack && p_brack < q_brack)
	{
		len = q_brack - p_brack + INIFILE_SPACE_CHAR_LEN;
		ret = storage_char_memory(&(p_section->sec),p_brack,len);
		if(ret < 0)
			goto error_out;
		return index;
	}
	return INIFILE_FORMAT_ERROR;
error_out:
	p_section->flag = STRUCT_NODE_UN_USED;
	return ret;
}

int init_every_line_inifile(int ini_fd,char *line)
{
	int ret = 0;
	INI_FILE_LINE_TYPE type;
	INI_FILE *p_file = ini_array.ini_array;
	p_file += ini_fd;
	INI_SETION *p_section = p_file->head,*n_section = section_array.section_array;
	INI_KEYVALUE *p_key = NULL,*n_key = keyval_array.keyval_array;
	
	type = judge_ini_file_linetype(line);
	switch(type)
	{
		case LINE_SECTION:
			ret = init_line_section(line);
			if(ret >= 0)
			{
				n_section += ret;
				if(p_section == NULL)
					p_file->head = n_section;
				else{
					while(p_section->last)
						p_section = p_section->last;
					p_section->last = n_section;
					n_section->pre = p_section;
					n_section->last = NULL;
				}
			}
			break;
		case LINE_KEYVALUE:
			ret = init_line_key_val(line);
			if(ret >= 0)
			{
				n_key += ret;
				if(p_section == NULL)
				{
					n_key->flag = STRUCT_NODE_UN_USED;
					ret = INIFILE_FORMAT_ERROR;
				}else{
					while(p_section->last)
						p_section = p_section->last;
					p_key = p_section->head;
					if(p_key == NULL)
						p_section->head = n_key;
					else{
						while(p_key->last)
							p_key= p_key->last;
						p_key->last = n_key;
						n_key->pre = p_key;
						n_key->last = NULL;
					}
				}
			}
			break;
		case LINE_NOTE:
			break;
		case LINE_BLANK:
			break;
		case LINE_ERROR:
			ret = INIFILE_FORMAT_ERROR;
			break;
		default:
			ret = INIFILE_FORMAT_ERROR;
			break;
	}
	return ret;
}


int read_analys_ini_file(int ini_fd,const char *file_name)
{
	int ret = 0;
	char line[INIFILE_MAX_CONTENT_LINELEN] = {0};
	FILE *fp = fopen(file_name,"r");

	if(fp == NULL)
	{
		PERROR("File no exise of can't open\n");
		return INIFILE_NO_EXIST;
	}
	
	while(fgets(line,(INIFILE_MAX_CONTENT_LINELEN),fp) != NULL)
	{
		ret = init_every_line_inifile(ini_fd,line);
		if(ret < 0)
			break;
		memset(line,0,INIFILE_MAX_CONTENT_LINELEN);
	}
	fclose(fp);
	return ret;	
}

void delete_tmp_name_file(const char *filename)
{
	char cmd_str[INIFILE_SYSTEM_CMD_LEN] = {0};
	sprintf(cmd_str,"rm -rf %s%s",filename,tem_suff_name);
	system (cmd_str);
}

void re_name_ini_file(const char *filename)
{
	char cmd_str[INIFILE_SYSTEM_CMD_LEN] = {0};
	sprintf(cmd_str,"rm -rf %s && mv %s%s %s",filename,filename,tem_suff_name,filename);
	system (cmd_str);
}

int re_write_ini_file(const char *filename)
{
	int len,i;
	char tmp_name[INIFILE_FILEALL_NAME_LEN] = {0},*p_char = NULL;
	FILE *rp = NULL,*wp = NULL;
	char line[INIFILE_MAX_CONTENT_LINELEN] = {0};

	sprintf(tmp_name,"%s%s",filename,tem_suff_name);
	
	rp = fopen(filename,"r");
	if(rp == NULL)
	{
		return INIFILE_NO_EXIST;
	}
	wp = fopen(tmp_name,"w");
	if(wp == NULL)
	{
		return INIFILE_NO_EXIST;
	}

	while(fgets(line,INIFILE_MAX_CONTENT_LINELEN,rp) != NULL)
	{
		len = str_int_len(line);
		p_char = line;
		for(i = 0;i < len;i++)
		{
			printf(" %d ",*p_char);
			p_char++;
		}
		printf("\n");
		fwrite(line,INIFILE_SPACE_CHAR_LEN,len,wp);
		fwrite(line,INIFILE_SPACE_CHAR_LEN,len,wp);
		memset(line,0,INIFILE_MAX_CONTENT_LINELEN);
	}
	
	fclose(rp);
	fclose(wp);
	return 0;
}

int rewrite_after_update(const char *filename,INI_PARAMETER *parameter)
{
	int ret = 0,len = 0,flag = 0,mod_flag = 0;
	FILE *rp = NULL,*wp = NULL;
	INI_FILE_LINE_TYPE type;
	char tmp_name[INIFILE_FILEALL_NAME_LEN] = {0},*p_char = NULL,*q_char = NULL,*p_value = NULL,*p_comma = NULL;
	char line[INIFILE_MAX_CONTENT_LINELEN] = {0},mod_line[INIFILE_MAX_CONTENT_LINELEN] = {0};

	if(parameter->section == NULL || parameter->key == NULL || parameter->value == NULL)
		return INIFILE_ERROR_PARAMETER;

	if(parameter->section_len <= 0)
		parameter->section_len = str_int_len(parameter->section);
	if(parameter->key_len <= 0)
		parameter->key_len = str_int_len(parameter->key);
	if(parameter->value_len <= 0)
		parameter->value_len = str_int_len(parameter->value);
	sprintf(tmp_name,"%s%s",filename,tem_suff_name);
	rp = fopen(filename,"r");
	if(rp == NULL)
	{
		ret = INIFILE_NO_EXIST;
		goto error_out;
	}
	wp = fopen(tmp_name,"w");
	if(wp == NULL)
	{
		ret = INIFILE_NO_EXIST;
		goto error_out;
	}

	while(fgets(line,INIFILE_MAX_CONTENT_LINELEN,rp) != NULL)
	{
		if(mod_flag== 0)
		{
			type = judge_ini_file_linetype(line);
			switch(type)
			{
				case LINE_SECTION:
					p_char = str_frist_constchar(line,CONTENT_LEFT_BRACKET);
					if(p_char == NULL)
					{
						ret = INIFILE_REWRITE_ERROR;
						goto error_out;
					}else
						p_char++;
					q_char = p_char + parameter->section_len;
					if(*q_char == CONTENT_RIGHT_BRACKET)
					{
						ret = memcmp((const void *)(parameter->section),(const void *)p_char,parameter->section_len);
						if(ret == 0)
							flag = 1;
					}
					break;
				case LINE_KEYVALUE:
					if(flag == 1)
					{
						p_value = str_frist_constchar(line,CONTENT_EQUALITY_SIGN);
						if(p_value == NULL)
						{
							ret = INIFILE_REWRITE_ERROR;
							goto error_out;
						}else{
							p_char = line;
							while(*p_char == CONTENT_SPACE_SIGN || *p_char == CONTENT_TAB_SIGN)
							{
								p_char++;
							}
						}

						q_char = p_char + parameter->key_len;
						if(*q_char == CONTENT_SPACE_SIGN || *q_char == CONTENT_TAB_SIGN || *q_char == CONTENT_EQUALITY_SIGN)
						{
							ret = memcmp((const void *)(parameter->key),(const void *)p_char,parameter->key_len);
							if(ret == 0)
							{
								while(*p_value == CONTENT_SPACE_SIGN || *p_value == CONTENT_TAB_SIGN)
								{
									p_value++;
								}
								len = p_value - line;
								memcpy(mod_line,line,len);
								p_char = find_frist_endchar(mod_line);
							
								len = parameter->value_len + 1;
								snprintf(p_char,len,"%s",parameter->value);
								p_comma = str_frist_constchar(p_value,CONTENT_COMMA_SIGN);
								if(p_comma)
								
{
									p_char = find_frist_endchar(mod_line);
									sprintf(p_char,"\t%s",p_comma);
								}
								memset(line,0,INIFILE_MAX_CONTENT_LINELEN);
								sprintf(line,"%s\n",mod_line);
								mod_flag = 1;
							}
						}
					}
					break;
				case LINE_NOTE:
					break;
				case LINE_BLANK:
					break;
				case LINE_ERROR:
					ret = INIFILE_FORMAT_ERROR;
					goto error_out;
					break;
			}
		}
		len = str_int_len(line);
		fwrite(line,INIFILE_SPACE_CHAR_LEN,len,wp);
		memset(line,0,INIFILE_MAX_CONTENT_LINELEN);
	}
	ret = 0;
error_out:
	if(rp)
		fclose(rp);
	if(wp)
		fclose(wp);
	if(ret < 0)
		delete_tmp_name_file(filename);
	return ret;
}

int rewrite_after_add(const char *filename,INI_PARAMETER *parameter)
{
	int ret = 0,len = 0,flag = 0;
	FILE *rp = NULL,*wp = NULL;
	char tmp_name[INIFILE_FILEALL_NAME_LEN] = {0},*p_char = NULL,*q_char = NULL;
	char line[INIFILE_MAX_CONTENT_LINELEN] = {0};
	INI_FILE_LINE_TYPE type;

	if(parameter->section == NULL)
		return INIFILE_ERROR_PARAMETER;
	if(parameter->section_len <= 0)
		parameter->section_len = str_int_len(parameter->section);
	sprintf(tmp_name,"%s%s",filename,tem_suff_name);
	rp = fopen(filename,"r");
	if(rp == NULL)
	{
		ret = INIFILE_NO_EXIST;
		goto error_out;
	}
	wp = fopen(tmp_name,"w");
	if(wp == NULL)
	{
		ret = INIFILE_NO_EXIST;
		goto error_out;
	}

	if(parameter->key == NULL)
	{
		len = parameter->section_len + 1;
		p_char = line;
		*p_char = CONTENT_LEFT_BRACKET;
		p_char++;
		snprintf(p_char,len,"%s",parameter->section);
		p_char = find_frist_endchar(p_char);
		sprintf(p_char,"]\n\n");
		len = str_int_len(line);
		fwrite(line,INIFILE_SPACE_CHAR_LEN,len,wp);
		memset(line,0,INIFILE_MAX_CONTENT_LINELEN);
		flag = 1;
	}else{
		if(parameter->key_len <= 0)
			parameter->key_len = str_int_len(parameter->key);
		if(parameter->value_len <= 0)
			parameter->value_len = str_int_len(parameter->value);
	}
	
	while(fgets(line,INIFILE_MAX_CONTENT_LINELEN,rp) != NULL)
	{
		if(flag == 0)
		{
			type = judge_ini_file_linetype(line);
			switch(type)
			{
				case LINE_SECTION:
					p_char = str_frist_constchar(line,CONTENT_LEFT_BRACKET);
					if(p_char == NULL)
					{
						ret = INIFILE_REWRITE_ERROR;
						goto error_out;
					}else
						p_char++;
					
					q_char = p_char + parameter->section_len;
					if(*q_char == CONTENT_RIGHT_BRACKET)
					{
						ret = memcmp((const void *)(parameter->section),(const void *)p_char,parameter->section_len);
						if(ret == 0)
						{
							p_char = find_frist_endchar(p_char);
							len = parameter->key_len + 1;
							snprintf(p_char,len,"%s",parameter->key);
							p_char = find_frist_endchar(p_char);
							sprintf(p_char,"\t=\t");
							p_char = find_frist_endchar(p_char);
							len = parameter->value_len + 1;
							snprintf(p_char,len,"%s",parameter->value);
							p_char = find_frist_endchar(p_char);
							*p_char = CONTENT_NEELINE_SIGN;
							flag = 1;
						
}
					}
					break;
				case LINE_KEYVALUE:
					break;
				case LINE_NOTE:
					break;
				case LINE_BLANK:
					break;
				case LINE_ERROR:
					ret = INIFILE_FORMAT_ERROR;
					goto error_out;
					break;
			}
		}
		len = str_int_len(line);
		fwrite(line,INIFILE_SPACE_CHAR_LEN,len,wp);
		memset(line,0,INIFILE_MAX_CONTENT_LINELEN);
	}
	ret = 0;
error_out:
	if(rp)
		fclose(rp);
	if(wp)
		fclose(wp);
	if(ret < 0)
		delete_tmp_name_file(filename);
	return ret;
}

int rewrite_after_delete(const char *filename,INI_PARAMETER *parameter)
{
	int ret = 0,flag = 0,len = 0;
	FILE *rp = NULL,*wp = NULL;
	char tmp_name[INIFILE_FILEALL_NAME_LEN] = {0},*p_char = NULL,*q_char = NULL;
	char line[INIFILE_MAX_CONTENT_LINELEN] = {0},*p_value = NULL;
	INI_FILE_LINE_TYPE type;

	if(parameter->section == NULL)
		return INIFILE_ERROR_PARAMETER;
	if(parameter->section_len <= 0)
		parameter->section_len = str_int_len(parameter->section);
	sprintf(tmp_name,"%s%s",filename,tem_suff_name);
	rp = fopen(filename,"r");
	if(rp == NULL)
	{
		ret = INIFILE_NO_EXIST;
		goto error_out;
	}
	wp = fopen(tmp_name,"w");
	if(wp == NULL)
	{
		ret = INIFILE_NO_EXIST;
		goto error_out;
	}

	while(fgets(line,INIFILE_MAX_CONTENT_LINELEN,rp) != NULL)
	{
		if(flag != 3)
		{
			type = judge_ini_file_linetype(line);
			switch(type)
			{
				case LINE_SECTION:
					p_char = str_frist_constchar(line,CONTENT_LEFT_BRACKET);
					if(p_char == NULL)
					{
						ret = INIFILE_REWRITE_ERROR;
						goto error_out;
					}else
						p_char++;

					if(flag == 1)
					{
						flag = 3;
						break;
					}else if(flag ==2 )
					{
						PERROR("There is no found key value under section\n");	
						flag = 3;
						break;
					}
					
					q_char = p_char + parameter->section_len;
					if(*q_char == CONTENT_RIGHT_BRACKET)
					{
						ret = memcmp((const void *)(parameter->section),(const void *)p_char,parameter->section_len);
						if(ret == 0)
						{
							if(parameter->key)
							{
								if(parameter->key_len <= 0)
									parameter->key_len = str_int_len(parameter->key);
								flag = 2;
							}else{
								flag = 1;
							}
							continue;
						
}
					}
					break;
				case LINE_KEYVALUE:
					if (flag == 1)
						continue;
					else if(flag == 2)
					{
						p_value = str_frist_constchar(line,CONTENT_EQUALITY_SIGN);
						if(p_value == NULL)
						{
							ret = INIFILE_REWRITE_ERROR;
							goto error_out;
						}else{
							p_char = line;
							while(*p_char == CONTENT_SPACE_SIGN || *p_char == CONTENT_TAB_SIGN)
							{
								p_char++;
							}
						}

						q_char = p_char + parameter->key_len;
						if(*q_char == CONTENT_SPACE_SIGN || *q_char == CONTENT_TAB_SIGN || *q_char == CONTENT_EQUALITY_SIGN)
						{
							ret = memcmp((const void *)(parameter->key),(const void *)p_char,parameter->key_len);
							if(ret == 0)
							{
								flag = 3;
								continue;
							}
						}
					}
					break;
				case LINE_NOTE:
					break;
				case LINE_BLANK:
					break;
				case LINE_ERROR:
					ret = INIFILE_FORMAT_ERROR;
					goto error_out;
					break;
			}
		}
		len = str_int_len(line);
		fwrite(line,INIFILE_SPACE_CHAR_LEN,len,wp);
		memset(line,0,INIFILE_MAX_CONTENT_LINELEN);
	}
error_out:
	if(rp)
		fclose(rp);
	if(wp)
		fclose(wp);
	if(ret < 0)
		delete_tmp_name_file(filename);
	return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int init_ini_file(const char *filename,int len)
{
	int index,ret;
	INI_FILE *p_file = ini_array.ini_array;

	ini_file_memory_init();
	
	if(filename == NULL)
		return INIFILE_ERROR_PARAMETER;
	
	index = get_index_ini_file();
	if(index < 0)
		return index;
	
	p_file += index;
	if(len <= 0)
		len = str_int_len(filename);

	ret = storage_char_memory(&(p_file->name),filename,len);
	if(ret < 0)
	{
		destroy_ini_source(index);
		return ret;
	}

	ret = read_analys_ini_file(index,filename);
	if(ret < 0)
	{
		destroy_ini_source(index);
		return ret;
	}

	return index;
}

const char *get_value_ofkey(int ini_fd,INI_PARAMETER *parameter)
{
	const char *value = NULL;
	INI_FILE *p_ini_file = ini_array.ini_array;
	INI_SETION *p_section = NULL;
	INI_KEYVALUE *p_keyval = NULL;
	
	if(ini_fd < 0 || ini_fd >= MAX_OPERATION_FILE_NUM)
	{
		PERROR("There is something wrong with inside paramter:::%d\n",ini_fd);
		return;
	}
	p_ini_file += ini_fd;
	pthread_mutex_lock(&p_ini_file->file_lock);
	p_section = find_inifile_section(p_ini_file->head,parameter->section,parameter->section_len);
	if(p_section == NULL)
	{
		value = NULL;
		goto error_out;
	}
	p_keyval = find_keyvalue_unsect(p_section->head,parameter->key,parameter->key_len);
	if(p_keyval == NULL)
	{
		value = NULL;
		goto error_out;
	}
	value = p_keyval->value;
error_out:
	pthread_mutex_unlock(&p_ini_file->file_lock);
	return value;
}

int update_value_ofkey(int ini_fd,INI_PARAMETER *parameter)
{
	int ret;
	INI_FILE *p_ini_file = ini_array.ini_array;
	INI_SETION *p_section = NULL;
	INI_KEYVALUE *p_keyval = NULL;
	if(ini_fd < 0 || ini_fd >= MAX_OPERATION_FILE_NUM || parameter->section || parameter->key || parameter->value)
	{
		PERROR("There is something wrong with inside paramter:::%d\n",ini_fd);
		return INIFILE_ERROR_PARAMETER;
	}
	p_ini_file += ini_fd;
	pthread_mutex_lock(&p_ini_file->file_lock);
	p_section = find_inifile_section(p_ini_file->head,parameter->section,parameter->section_len);
	if(p_section == NULL)
	{
		ret = INIFILE_SECTION_NOFOUND;
		goto error_out;
	}
	p_keyval = find_keyvalue_unsect(p_section->head,parameter->key,parameter->key_len);
	if(p_keyval == NULL)
	{
		ret = INIFILE_KEYVALUE_NOFOUND;
		goto error_out;
	}

	if(parameter->value_len <= 0)
		parameter->value_len = str_int_len(parameter->value);

	ret = storage_char_memory(&p_keyval->value,parameter->value,parameter->value_len);
error_out:
	pthread_mutex_unlock(&p_ini_file->file_lock);
	return ret;
}

int add_value_ofkey(int ini_fd,INI_PARAMETER *parameter)
{
	int ret = 0,index = 0,len = 0;
	char line[INIFILE_MAX_CONTENT_LINELEN] = {0},*p_char = NULL;
	INI_FILE *p_ini_file = ini_array.ini_array;
	INI_SETION *p_section = NULL;
	INI_KEYVALUE *p_keyval = NULL,*p_key_end = NULL;
	if(ini_fd < 0 || ini_fd >= MAX_OPERATION_FILE_NUM || parameter->section || parameter->key || parameter->value)
	{
		PERROR("There is something wrong with inside paramter:::%d\n",ini_fd);
		return INIFILE_ERROR_PARAMETER;
	}
	p_ini_file += ini_fd;
	pthread_mutex_lock(&p_ini_file->file_lock);
	p_section = find_inifile_section(p_ini_file->head,parameter->section,parameter->section_len);
	if(p_section == NULL)
	{
		ret = INIFILE_SECTION_NOFOUND;
		goto error_out;
	}
	p_keyval = find_keyvalue_unsect(p_section->head,parameter->key,parameter->key_len);
	if(p_keyval)
	{
		ret = INIFILE_SECTION_ALREAD;
		goto error_out;
	}
	if(parameter->key_len <= 0)
		parameter->key_len  = str_int_len(parameter->key);

	if(parameter->value_len <= 0)
		parameter->value_len = str_int_len(parameter->value);
	
	p_char = line;
	len = parameter->key_len + 1;
	snprintf(p_char,len,"%s",parameter->key);
	p_char = find_frist_endchar(p_char);
	*p_char = CONTENT_EQUALITY_SIGN;
	p_char++;
	len = parameter->value_len + 1;
	snprintf(p_char,len,"%s",parameter->value);
	
	//sprintf(line,"%s=%s",parameter->key,parameter->value);
	p_keyval = keyval_array.keyval_array;
	ret = init_line_key_val(line);
	if(ret < 0)
		goto error_out;
	p_keyval += ret;
	if(p_section->head)
	{
		p_key_end = p_section->head;
		while(p_key_end->last)
		{
			p_key_end = p_key_end->last;
		}
		p_key_end->last = p_keyval;
		p_keyval->pre = p_key_end;
	}else{
		p_section->head = p_keyval;
	}
	ret = rewrite_after_add(p_ini_file->name,parameter);
	if(ret < 0)
		goto error_out;
	ret = rewrite_after_update(p_ini_file->name,parameter);
error_out:
	pthread_mutex_unlock(&p_ini_file->file_lock);
	return ret;
}

int delete_value_ofkey(int ini_fd,INI_PARAMETER *parameter)
{
	int ret;
	INI_FILE *p_ini_file = ini_array.ini_array;
	INI_SETION *p_section = NULL;
	INI_KEYVALUE *p_keyval = NULL;
	if(ini_fd < 0 || ini_fd >= MAX_OPERATION_FILE_NUM || parameter->section || parameter->key )
	{
		PERROR("There is something wrong with inside paramter:::%d\n",ini_fd);
		return INIFILE_ERROR_PARAMETER;
	}
	p_ini_file += ini_fd;
	pthread_mutex_lock(&p_ini_file->file_lock);
	p_section = find_inifile_section(p_ini_file->head,parameter->section,parameter->section_len);
	if(p_section == NULL)
	{
		ret = INIFILE_SECTION_NOFOUND;
		goto error_out;
	}
	p_keyval = find_keyvalue_unsect(p_section->head,parameter->key,parameter->key_len);
	if(p_keyval == NULL)
	{
		ret = INIFILE_KEYVALUE_NOFOUND;
		goto error_out;
	}
	if(p_keyval->pre)
	{
		p_keyval->pre->last = p_keyval->last;
	}else{
		p_section->head = p_keyval->last;
	}
	if(p_keyval->last)
	{
		p_keyval->last->pre = p_keyval->pre;
	}else{
		if(p_keyval->pre)
		{
			p_keyval->pre->last = NULL;
		}else{
			p_section->head = NULL;
		}
	}
	p_keyval->pre = NULL;
	p_keyval->last = NULL;
	p_keyval->key = NULL;
	p_keyval->value = NULL;
	p_keyval->flag = STRUCT_NODE_UN_USED;
	ret = rewrite_after_delete(p_ini_file->name,parameter);
error_out:
	pthread_mutex_unlock(&p_ini_file->file_lock);
	return ret;
}

int delete_ini_section(int ini_fd,INI_PARAMETER *parameter)
{
	int ret;
	INI_FILE *p_ini_file = ini_array.ini_array;
	INI_SETION *p_section = NULL;
	INI_KEYVALUE *p_keyval = NULL;
	if(ini_fd < 0 || ini_fd >= MAX_OPERATION_FILE_NUM || parameter->section || parameter->key || parameter->value)
	{
		PERROR("There is something wrong with inside paramter:::%d\n",ini_fd);
		return INIFILE_ERROR_PARAMETER;
	}
	p_ini_file += ini_fd;
	pthread_mutex_lock(&p_ini_file->file_lock);
	p_section = find_inifile_section(p_ini_file->head,parameter->section,parameter->section_len);
	if(p_section == NULL)
	{
		ret = INIFILE_SECTION_NOFOUND;
		goto error_out;
	}
	if(p_section->pre)
	{
		p_section->pre->last = p_section->last;
	}else{
		p_ini_file->head = p_section->last;
	}
	if(p_section->last)
	{
		p_section->last->pre = p_section->pre;
	}else{
		if(p_section->pre)
		{
			p_section->pre->last = NULL;
		}else{
			p_ini_file->head = NULL;
		}
	}

	p_section->pre = NULL;
	p_section->last = NULL;
	p_section->sec = NULL;
	p_keyval = p_section->head;
	p_section->head = NULL;
	p_section->flag = STRUCT_NODE_UN_USED;
	while(p_keyval)
	{
		p_keyval->key = NULL;
		p_keyval->value = NULL;
		p_keyval->pre = NULL;
		if(p_keyval->last)
		{
			p_keyval = p_keyval->last;
			p_keyval->pre->last = NULL;
			p_keyval->pre->flag = STRUCT_NODE_UN_USED;
		}else{
			p_keyval->flag = STRUCT_NODE_UN_USED;
			p_keyval = NULL;
		}
	}
	ret = rewrite_after_delete(p_ini_file->name,parameter);
error_out:
	pthread_mutex_unlock(&p_ini_file->file_lock);
	return ret;
}

int add_ini_section(int ini_fd,INI_PARAMETER *parameter)
{
	int ret = 0,len = 0;
	char line[INIFILE_MAX_CONTENT_LINELEN] = {0},*p_char = NULL;
	INI_FILE *p_ini_file = ini_array.ini_array;
	INI_SETION *p_section = NULL,*p_sec_end = NULL;
	INI_KEYVALUE *p_keyval = NULL;
	if(ini_fd < 0 || ini_fd >= MAX_OPERATION_FILE_NUM || parameter->section || parameter->key || parameter->value)
	{
		PERROR("There is something wrong with inside paramter:::%d\n",ini_fd);
		return INIFILE_ERROR_PARAMETER;
	}
	p_ini_file += ini_fd;
	pthread_mutex_lock(&p_ini_file->file_lock);
	p_section = find_inifile_section(p_ini_file->head,parameter->section,parameter->section_len);
	if(p_section)
	{
		ret = INIFILE_SECTION_ALREAD;
		goto error_out;
	}
	if(parameter->section_len <= 0)
		parameter->section_len = str_int_len(parameter->section);
	p_char = line;
	*p_char = CONTENT_LEFT_BRACKET;
	p_char++;
	len = parameter->section_len + 1;
	snprintf(p_char,len,"%s",parameter->section);
	p_char = find_frist_endchar(p_char);
	*p_char = CONTENT_RIGHT_BRACKET;
	//sprintf(line,"[%s]",parameter->section);
	ret =init_line_section(line);
	if(ret < 0)
		goto error_out;
	p_section = section_array.section_array;
	p_section += ret;
	if(p_ini_file->head == NULL)
		p_ini_file->head = p_section;
	else{
		p_sec_end = p_ini_file->head;
		while(p_sec_end->last)
			p_sec_end = p_sec_end->last;
		p_sec_end->last = p_section;
		p_section->pre = p_sec_end;
	}
	ret = rewrite_after_add(p_ini_file->name,parameter);
error_out:
	pthread_mutex_unlock(&p_ini_file->file_lock);
	return ret;
}

void destroy_ini_source(int ini_fd)
{
	INI_FILE *p_ini_file = ini_array.ini_array;
	INI_SETION *p_section = NULL;
	INI_KEYVALUE *p_keyval = NULL;
	
	if(ini_fd < 0 || ini_fd >= MAX_OPERATION_FILE_NUM)
	{
		PERROR("There is something wrong with inside paramter:::%d\n",ini_fd);
		return;
	}

	p_ini_file += ini_fd;
	if(p_ini_file->flag == STRUCT_NODE_IN_USED)
	{
		p_section = p_ini_file->head;
		while(p_section)
		{
			p_keyval = p_section->head;
			while(p_keyval)
			{
				p_keyval->key = NULL;
				p_keyval->value = NULL;
				p_keyval->pre = NULL;
				if(p_keyval->last)
				{
					p_keyval = p_keyval->last;
					p_keyval->pre->last = NULL;
					p_keyval->pre->flag = STRUCT_NODE_UN_USED;
				}else{
					p_keyval->flag = STRUCT_NODE_UN_USED;
					p_keyval = NULL;
				}
			}
			p_section->sec = NULL;
			p_section->head = NULL;
			p_section->pre = NULL;
			if(p_section->last)
			{
				p_section = p_section->last;
				p_section->pre->last = NULL;
				p_section->pre->flag = STRUCT_NODE_UN_USED;
			}else{
				p_section->flag = STRUCT_NODE_UN_USED;
				p_section = NULL;
			}
		}
		p_ini_file->name = NULL;
		p_ini_file->head = NULL;
		p_ini_file->flag = STRUCT_NODE_UN_USED;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
///@test
///

void ini_file_info_out(int ini_fd)
{
	INI_FILE *p_ini_file = ini_array.ini_array;
	INI_SETION *p_section = NULL;
	INI_KEYVALUE *p_keyval = NULL;

	if(ini_fd < 0 || ini_fd >= MAX_OPERATION_FILE_NUM)
		return;
	
	p_ini_file += ini_fd;
	if(p_ini_file->flag == STRUCT_NODE_IN_USED)
	{
		if(p_ini_file->name)
			PDEBUG("ini file name ::%s \n",p_ini_file->name);
	
		p_section = p_ini_file->head;
		while(p_section)
		{
			if(p_section->sec)
				PDEBUG("ini file section :: %s \n",p_section->sec);
			p_keyval = p_section->head;
			while(p_keyval)
			{
				PDEBUG("ini file key value : %s = %s \n",p_keyval->key,p_keyval->value);
				p_keyval = p_keyval->last;
			}
			p_section = p_section->last;
		}
	}
}

int main(int argc, char *argv[])
{
	int fd = 0;
	fd = init_ini_file("test.ini",0);
	if(fd < 0)
		return fd;
	ini_file_info_out(fd);
	destroy_ini_source(fd);
	return 0;
}

