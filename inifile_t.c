
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
#define IS_SHOWDEBUG_MESSAGE			0

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
	LINE_UNKNOWN,
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
}INI_FILE;

typedef struct ini_file_array{
	INI_FILE ini_array[MAX_OPERATION_FILE_NUM];
	pthread_mutex_t array_lock;
}INI_FILE_ARRAY;

static MEMORY_POOL pool_mem = {0};
static INI_FILE_ARRAY ini_array = {0};
static INI_SECTION_ARRAY section_array = {0};
static INI_KEYVALUE_ARRAY keyval_array = {0};

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
	if(pool_mem.current == NULL)
	{
		pool_mem.current = pool_mem.pool;
		pool_mem.curr_len = 0;
		pthread_mutex_init(&pool_mem.pool_lock, NULL);
		pthread_mutex_init(&ini_array.array_lock, NULL); 
		pthread_mutex_init(&section_array.array_lock, NULL); 
		pthread_mutex_init(&keyval_array.array_lock, NULL); 
	}
}

int ini_file_memory_adjust()
{
	int i,len,ret = 0;
	INI_FILE *p_ini_file = ini_array.ini_array;
	INI_SETION *p_section = NULL;
	INI_KEYVALUE *p_keyval = NULL;
	pthread_mutex_lock(&ini_array.array_lock);
	
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
	pthread_mutex_lock(&ini_array.array_lock);
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
	pthread_mutex_lock(&ini_array.array_lock);
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
	pthread_mutex_lock(&section_array.array_lock);
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
	pthread_mutex_lock(&keyval_array.array_lock);
	return i;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

INI_FILE_LINE_TYPE judge_ini_file_linetype(char *line)
{
	char *pp = line,*p_brack = NULL,*q_brack = NULL,*p_equ = NULL,*p_comma = NULL;

	if(*pp == CONTENT_NEELINE_SIGN)
		return LINE_UNKNOWN;
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

int init_line_section(char *line)
{
	int index = 0,len = 0,ret = 0;
	char *pp = line,*p_brack = NULL,*q_brack = NULL;
	INI_SETION *p_section = section_array.section_array;
	
	p_brack = str_frist_constchar(line,CONTENT_LEFT_BRACKET);
	q_brack = str_frist_constchar(line,CONTENT_RIGHT_BRACKET);

	index = get_index_ini_keyvalue();
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
			if(ret > 0)
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
		case LINE_UNKNOWN:
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

int re_write_ini_file(const char *filename)
{
	int len;
	char tmp_name[128] = {0};
	FILE *rp = NULL,*wp = NULL;
	char line[INIFILE_MAX_CONTENT_LINELEN] = {0};

	sprintf(tmp_name,"%s.tmp",filename);

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

	while(fgets(line,(INIFILE_MAX_CONTENT_LINELEN),rp) != NULL)
	{
		fwrite(line,INIFILE_SPACE_CHAR_LEN,len,wp);
	}
	fclose(rp);
	fclose(wp);
	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int init_ini_file(const char *filename,int len)
{
	int index,ret;
	INI_FILE *p_file = ini_array.ini_array;
	
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
				p_keyval = p_keyval->last;
				p_keyval->pre->last = NULL;
				p_keyval->pre->flag = STRUCT_NODE_UN_USED;
			}
			p_section->sec = NULL;
			p_section->head = NULL;
			p_section->pre = NULL;
			p_section = p_section->last;
			p_section->pre->last = NULL;
			p_section->pre->flag = STRUCT_NODE_UN_USED;
		}
		p_ini_file->name = NULL;
		p_ini_file->head = NULL;
		p_ini_file->flag = STRUCT_NODE_UN_USED;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
///@test
///

int main(int argc, char *argv[])
{
	PERROR("hello world\n");
	return 0;
}

