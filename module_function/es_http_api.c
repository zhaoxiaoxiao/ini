
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>

#include "common.h"
#include "socket_api.h"
#include "frame_tool.h"
#include "http_applayer.h"
#include "es_http_api.h"

#define ES_MEM_POLL_SIZE			(1536)

#define SOCKET_BUFF_LEN				(1024)
#define AN_SOCKET_BUFF_LEN			(SOCKET_BUFF_LEN * 2)
#define SOCKET_BUF_RECV				(SOCKET_BUFF_LEN - 1)

#define ES_RESPOND_ARRAY_LEN		(10)

#define ES_RESPOND_ARRAY_ALL_LEN	(ES_RESPOND_ARRAY_LEN+1)

static const char *verb_str_arr[] = {
	"GET",
	"POST",
	"PUT",
	"HEAD",
	"DELETE"
};

static const char *protocol_str[] = {
	"HTTP/1.1",
	"HTTPS/1.1"
};

typedef struct es_server_info{
	char ip_addr[16];
	int es_fd_;
	int es_fd_b;
	int is_run;
	
	int insert_index,send_index,recv_index,call_index;
	sem_t free_num,send_num,recv_num,call_num;
	
	PROTOCOL_TYPE type_;
	unsigned short port_;

	pthread_mutex_t asyn_mutex;
	pthread_mutex_t bloc_mutex;
}ES_SERVER_INFO;

static ES_SERVER_INFO es_info = {0};
static ES_RESPOND es_res_array[ES_RESPOND_ARRAY_ALL_LEN] = {0};

////////////////////////////////////////////////////////////////////////////////////////////////////////
int es_server_connect(int fd)
{
	return connect_tcp_server(fd,es_info.port_,es_info.ip_addr);
}

int get_insert_index()
{
	int index = 0;
	sem_wait(&es_info.free_num);
	index = es_info.insert_index;
	es_info.insert_index++;
	if(es_info.insert_index >= ES_RESPOND_ARRAY_LEN)
		es_info.insert_index = 0;
	return index;
}

int get_send_index()
{
	int index = 0;
	if(es_info.is_run == 0)
	{
		sem_getvalue(&es_info.send_num,&index);
		if(index == 0)
			return -1;
	}
	sem_wait(&es_info.send_num);
	index = es_info.send_index;
	es_info.send_index++;
	if(es_info.send_index >= ES_RESPOND_ARRAY_LEN)
		es_info.send_index = 0;
	return index;
}

int get_recv_index()
{
	int index = 0;
	if(es_info.is_run == 0)
	{
		sem_getvalue(&es_info.send_num,&index);
		if(index == 0)
		{
			sem_getvalue(&es_info.recv_num,&index);
			if(index == 0)
			{
				return -1;
			}
		}
	}
	sem_wait(&es_info.recv_num);
	index = es_info.recv_index;
	es_info.recv_index++;
	if(es_info.recv_index >= ES_RESPOND_ARRAY_LEN)
		es_info.recv_index = 0;
	return index;
}

int get_call_index()
{
	int index = 0;
	if(es_info.is_run == 0)
	{
		sem_getvalue(&es_info.send_num,&index);
		if(index == 0)
		{
			sem_getvalue(&es_info.recv_num,&index);
			if(index == 0)
			{
				sem_getvalue(&es_info.call_num,&index);
				if(index == 0)
					return -1;
			}
		}
	}
	sem_wait(&es_info.call_num);
	index = es_info.call_index;
	es_info.call_index++;
	if(es_info.call_index >= ES_RESPOND_ARRAY_LEN)
		es_info.call_index = 0;
	return index;
}

void memory_char_move_bit(char *begin,int bit,int be_size)
{
	int i = 0,buf_len = 0;
	char *p_char = begin,*q_char = begin;

	if(bit >= be_size)
	{
		PERROR("error paramter :: %d \n",bit);
		return;
	}

	buf_len = frame_strlen(begin);
	if(bit >= buf_len)
	{
		memset(begin,0,bit);
		return;
	}

	q_char += bit;
	buf_len = frame_strlen(q_char);
	if(buf_len >= be_size)
	{
		PERROR("error paramter :: %d \n",bit);
		return;
	}
	for(i = 0;i < buf_len;i++)
	{
		*p_char = *q_char;
		p_char++;
		q_char++;
	}

	buf_len = be_size - buf_len;
	memset(p_char,0,buf_len);
}

int analysis_http_head_str(char *str,ES_RESPOND *res,int* bit_move)
{
	int buf_len = 0,str_len = 0,ret = 0;
	char *p_char = str,*q_char = str,*p_colon = NULL,
		tmp_char[32] = {0},http_str[] = "HTTP",seq_str[] = "\r\n",cont_str[] = "Content-Length";
	KEY_VALUE_NODE *node = NULL;
	
	buf_len = frame_strlen(str);
	
	if(res->http == NULL)
	{
		res->http = (HTTP_HEAD_INFO *)ngx_pnalloc(res->mem,sizeof(HTTP_HEAD_INFO));
		if(res->http == NULL)
		{
			PERROR("system malloc error\n");
			return -1;
		}
		memset(res->http,0,sizeof(HTTP_HEAD_INFO));
		p_char = framestr_frist_constchar(p_char,32);
		if(p_char)
		{
			str_len = p_char - q_char;
			if(str_len > 0)
			{
				str_len++;
				res->http->ver = (char *)ngx_pnalloc(res->mem,str_len);
				if(res->http->ver == NULL)
				{
					PERROR("system malloc error\n");
					return -1;
				}
				memset(res->http->ver,0,str_len);
				str_len--;
				memcpy(res->http->ver,q_char,str_len);
			}

			q_char = p_char + 1;
			p_char = framestr_frist_constchar(q_char,32);
			if(p_char)
			{
				str_len = p_char - q_char;
				if(str_len > 0)
				{
					memset(tmp_char,0,32);
					memcpy(tmp_char,q_char,str_len);
					res->http->ret = atoi(tmp_char);
				}
			}else{
				PERROR("There is something wrong to find space division flag\n");
				p_char = q_char;
				ret = -2;
				goto error_out;
			}

			q_char = p_char + 1;
			p_char = framestr_first_conststr(q_char,seq_str);
			if(p_char)
			{
				str_len = p_char - q_char;
				if(str_len > 0)
				{
					res->http->note = (char *)ngx_pnalloc(res->mem,str_len);
					if(res->http->note == NULL)
					{
						PERROR("system malloc error\n");
						return -1;
					}
					memset(res->http->note,0,str_len);
					str_len--;
					memcpy(res->http->note,q_char,str_len);
				}
			}else{
				PERROR("There is something wrong to find space division flag\n");
				p_char = q_char;
				ret = -2;
				goto error_out;
			}

			q_char = p_char + 2;
			p_char = framestr_first_conststr(q_char,seq_str);
			while(p_char){
				p_colon = framestr_frist_constchar(q_char,58);//:::
				if(p_colon)
				{
					node = (KEY_VALUE_NODE *)ngx_pnalloc(res->mem,sizeof(KEY_VALUE_NODE));
					if(node == NULL)
					{
						PERROR("system malloc error\n"); 
						return -1;
					}
					memset(node,0,sizeof(KEY_VALUE_NODE));
					str_len = p_colon - q_char;
					if(str_len > 0)
					{
						str_len++;
						node->key_ = (char *)ngx_pnalloc(res->mem,str_len);
						if(node->key_  == NULL)
						{
							PERROR("system malloc error\n"); 
							return -1;
						}
						memset(node->key_,0,str_len);
						str_len--;
						memcpy(node->key_,q_char,str_len);
					}

					p_colon++;
					while(*p_colon == 32 || *p_colon == 9)// space and table skip
						p_colon++;
					str_len = p_char - p_colon;
					if(str_len > 0)
					{
						str_len++;
						node->value_ = (char *)ngx_pnalloc(res->mem,str_len);
						if(node->value_  == NULL)
						{
							PERROR("system malloc error\n"); 
							return -1;
						}
						memset(node->value_,0,str_len);
						str_len--;
						memcpy(node->value_,p_colon,str_len);
					}

					add_keyvalue_after_head(res->http->head_,node);
					node = NULL;
				} 

				q_char = p_char + 2;
				p_char = framestr_first_conststr(q_char,seq_str);
			}
			
			p_char = find_value_after_head(res->http->head_,cont_str);
			if(p_char)
				res->http->body_len = atoi(p_char);
			else{
				PERROR("there is something wrong to find Content-Length key \n");
				p_char = q_char;
				ret = -2;
				goto error_out;
			}
			*bit_move += q_char - str; 
			return 0;
		}else{
			PERROR("There is something wrong to find space division flag\n");
			p_char += 4;
			ret = -2;
			goto error_out;
		}
	}else{
		PERROR("this suitation may not happen any time.the respond http head need be null\n");
		return -3;
	}
error_out:
	p_char = framestr_first_conststr(p_char,http_str);
	if(p_char)
	{
		*bit_move += p_char - str; 
	}else{
		*bit_move += buf_len;
	}
	return ret;
}

int analysis_http_json_str(char *str,ES_RESPOND *res,int* bit_move)
{
	int buf_len = 0,str_len = 0,is_obj_end = 0,copy_len = 0,ret = 0,in_move = *bit_move;
	char *p_char = str,*q_char = str,*p_colon = NULL,*p_left_brack = NULL,*p_right_brack = NULL,*p_comma = NULL,*p_start_obj = NULL,
		*p_end_obj = NULL,*p_left_quot = NULL,*p_right_quot = NULL,*p_right_square = NULL,tmp_char[32] = {0},
		http_str[] = "HTTP",hit_str[] = "hits",total_str[] = "total",_index_str[] = "_index",_type_str[] = "_type",_id_str[] = "_id";
	KEY_VALUE_NODE *node = NULL;
	OBJ_INFO *obj_node = NULL;
	
	buf_len = frame_strlen(str);
	if(res->http == NULL)
	{
		PERROR("this suitation may not happen any time.the respond http head must no null\n");
		ret = -4;
		goto error_out;
	}

	if(res->message)///only message
	{
		copy_len = frame_strlen(res->message);
		p_char = frame_end_charstr(res->message);
		copy_len = res->http->body_len - copy_len;
		if(copy_len > buf_len)
		{
			*bit_move += buf_len;
			memcpy(p_char,q_char,buf_len);
			ret = 1;
		}else{
			*bit_move += copy_len;
			memcpy(p_char,q_char,copy_len);
			ret = 0;
		}
		
		if(frame_strlen(res->message) == res->http->body_len)
			PERROR("The message len right\n");
		else
			PERROR("The message len error");
		return ret;
	}else if(res->err_message)//only error message
	{
		copy_len = frame_strlen(res->err_message);
		p_char = frame_end_charstr(res->err_message);
		copy_len = res->http->body_len - copy_len;
		if(copy_len > buf_len)
		{
			*bit_move += buf_len;
			memcpy(p_char,q_char,buf_len);
			ret = 1;
		}else{
			*bit_move += copy_len;
			memcpy(p_char,q_char,copy_len);
			ret = 0;
		}
		if(frame_strlen(res->err_message) == res->http->body_len)
			PERROR("The err_message len right\n");
		else
			PERROR("The err_message len error");
		return ret;
	}else if(res->search)
	{
		{
			p_left_brack = framestr_frist_constchar(q_char,123);//{{
			while(p_left_brack)
			{
				p_char = p_left_brack;
				p_right_brack = framestr_frist_constchar(p_left_brack,125);//}}
				if(p_right_brack)
				{
					p_right_brack++;
					p_end_obj = framestr_frist_constchar(p_right_brack,125);//}}
					if(p_end_obj)
					{
						p_left_brack++;
						p_left_brack = framestr_frist_constchar(p_left_brack,123);//{{
						if(p_left_brack && p_left_brack < p_right_brack)
						{
							obj_node = (OBJ_INFO *)ngx_pnalloc(res->mem,sizeof(OBJ_INFO));
							if(obj_node == NULL)
							{
								PERROR("system malloc error\n");
								return -1;
							}
							
							p_char = framestr_first_conststr(p_char,_index_str);
							if(p_char && p_char < p_left_brack)
							{
								p_comma = framestr_frist_constchar(p_char,44);///,
								if(!p_comma || p_comma > p_left_brack)
									p_comma = p_left_brack;
								p_colon = framestr_frist_constchar(p_char,58);//:::
								if(p_colon && p_colon < p_comma)
								{
									p_left_quot = framestr_frist_constchar(p_colon,34);//"""
									if(!p_left_quot || p_left_quot > p_comma)
									{
										p_left_quot = framestr_start_digital_char(p_colon);
										if(p_left_quot && p_left_quot < p_comma)
										{
											p_right_quot = framestr_end_digital_char(p_left_quot);
											if(p_right_quot && p_right_quot <= p_comma)
											{
												str_len = p_right_quot - p_left_quot;
												if(str_len > 0)
												{
													str_len++;
													obj_node->index = (char *)ngx_pnalloc(res->mem,str_len);
													if(obj_node->index == NULL)
													{
														PERROR("system malloc error\n");
														return -1;
													}
													memset(obj_node->index,0,str_len);
													str_len--;
													memcpy(obj_node->index,p_left_quot,str_len);
												}
											}
										}
									}else{
										p_left_quot++;
										p_right_quot = framestr_frist_constchar(p_left_quot,34);//"""
										if(p_right_quot && p_right_quot < p_comma)
										{
											str_len = p_right_quot - p_left_quot;
											if(str_len > 0)
											{
												str_len++;
												obj_node->index = (char *)ngx_pnalloc(res->mem,str_len);
												if(obj_node->index == NULL)
												{
													PERROR("system malloc error\n");
													return -1;
												}
												memset(obj_node->index,0,str_len);
												str_len--;
												memcpy(obj_node->index,p_left_quot,str_len);
											}
										}
									}
								}

								p_char = p_comma + 1;
							}

							p_char = framestr_first_conststr(p_char,_type_str);
							if(p_char && p_char < p_left_brack)
							{
								p_comma = framestr_frist_constchar(p_char,44);///,
								if(!p_comma || p_comma > p_left_brack)
									p_comma = p_left_brack;
								p_colon = framestr_frist_constchar(p_char,58);//:::
								if(p_colon && p_colon < p_comma)
								{
									p_left_quot = framestr_frist_constchar(p_colon,34);//"""
									if(!p_left_quot || p_left_quot > p_comma)
									{
										p_left_quot = framestr_start_digital_char(p_colon);
										if(p_left_quot && p_left_quot < p_comma)
										{
											p_right_quot = framestr_end_digital_char(p_left_quot);
											if(p_right_quot && p_right_quot <= p_comma)
											{
												str_len = p_right_quot - p_left_quot;
												if(str_len > 0)
												{
													str_len++;
													obj_node->type = (char *)ngx_pnalloc(res->mem,str_len);
													if(obj_node->type == NULL)
													{
														PERROR("system malloc error\n");
														return -1;
													}
													memset(obj_node->type,0,str_len);
													str_len--;
													memcpy(obj_node->type,p_left_quot,str_len);
												}
											}
										}
									}else{
										p_left_quot++;
										p_right_quot = framestr_frist_constchar(p_left_quot,34);//"""
										if(p_right_quot && p_right_quot < p_comma)
										{
											str_len = p_right_quot - p_left_quot;
											if(str_len > 0)
											{
												str_len++;
												obj_node->type = (char *)ngx_pnalloc(res->mem,str_len);
												if(obj_node->type == NULL)
												{
													PERROR("system malloc error\n");
													return -1;
												}
												memset(obj_node->type,0,str_len);
												str_len--;
												memcpy(obj_node->type,p_left_quot,str_len);
											}
										}
									}
								}

								p_char = p_comma + 1;
							}

							p_char = framestr_first_conststr(p_char,_id_str);
							if(p_char && p_char < p_left_brack)
							{
								p_comma = framestr_frist_constchar(p_char,44);///,
								if(!p_comma || p_comma > p_left_brack)
									p_comma = p_left_brack;
								p_colon = framestr_frist_constchar(p_char,58);//:::
								if(p_colon && p_colon < p_comma)
								{
									p_left_quot = framestr_frist_constchar(p_colon,34);//"""
									if(!p_left_quot || p_left_quot > p_comma)
									{
										p_left_quot = framestr_start_digital_char(p_colon);
										if(p_left_quot && p_left_quot < p_comma)
										{
											p_right_quot = framestr_end_digital_char(p_left_quot);
											if(p_right_quot && p_right_quot <= p_comma)
											{
												str_len = p_right_quot - p_left_quot;
												if(str_len > 0)
												{
													str_len++;
													obj_node->id_ = (char *)ngx_pnalloc(res->mem,str_len);
													if(obj_node->id_ == NULL)
													{
														PERROR("system malloc error\n");
														return -1;
													}
													memset(obj_node->id_,0,str_len);
													str_len--;
													memcpy(obj_node->id_,p_left_quot,str_len);
												}
											}
										}
									}else{
										p_left_quot++;
										p_right_quot = framestr_frist_constchar(p_left_quot,34);//"""
										if(p_right_quot && p_right_quot < p_comma)
										{
											str_len = p_right_quot - p_left_quot;
											if(str_len > 0)
											{
												str_len++;
												obj_node->id_ = (char *)ngx_pnalloc(res->mem,str_len);
												if(obj_node->id_ == NULL)
												{
													PERROR("system malloc error\n");
													return -1;
												}
												memset(obj_node->id_,0,str_len);
												str_len--;
												memcpy(obj_node->id_,p_left_quot,str_len);
											}
										}
									}
								}

								p_char = p_comma + 1;
							}
							if(res->search->head == NULL)
							{
								add_objinfo_after_head(res->search->head,obj_node);
								res->search->tail = obj_node;
							}else{
								add_objinfo_after_tail(&(res->search->tail),obj_node);
							}
							
							p_char = p_left_brack;
							is_obj_end = 0;
							while(1)
							{
								p_comma = framestr_frist_constchar(p_char,44);///,
								if(!p_comma || p_comma > p_right_brack)
								{
									p_comma = p_right_brack;
									is_obj_end = 1;
								}

								p_colon = framestr_frist_constchar(p_char,58);//:::
								if(p_colon && p_colon < p_comma)
								{
									node = (KEY_VALUE_NODE *)ngx_pnalloc(res->mem,sizeof(KEY_VALUE_NODE));
									if(node == NULL)
									{
										PERROR("system malloc error\n");
										return -1;
									}
									memset(node,0,sizeof(KEY_VALUE_NODE));
									
									p_left_quot = framestr_frist_constchar(p_char,34);///,
									if(p_left_quot && p_left_quot < p_colon)
									{
										p_left_quot++;
										p_right_quot = framestr_frist_constchar(p_left_quot,34);///,
										if(p_right_quot && p_right_quot < p_colon)
										{
											str_len = p_right_quot - p_left_quot;
											if(str_len > 0)
											{
												str_len++;
												node->key_ = (char *)ngx_pnalloc(res->mem,str_len);
												if(node->key_ == NULL)
												{
													PERROR("system malloc error\n");
													return -1;
												}
												memset(node->key_,0,str_len);
												str_len--;
												memcpy(node->key_,p_left_quot,str_len);
											}
										}
									}

									p_left_quot = framestr_frist_constchar(p_colon,34);///,
									if(p_left_quot && p_left_quot < p_comma)
									{
										p_left_quot++;
										p_right_quot = framestr_frist_constchar(p_left_quot,34);///,
										if(p_right_quot && p_right_quot < p_comma)
										{
											str_len = p_right_quot - p_left_quot;
											if(str_len > 0)
											{
												str_len++;
												node->value_ = (char *)ngx_pnalloc(res->mem,str_len);
												if(node->value_ == NULL)
												{
													PERROR("system malloc error\n");
													return -1;
												}
												memset(node->value_,0,str_len);
												str_len--;
												memcpy(node->value_,p_left_quot,str_len);
											}
										}
									}else{
										p_left_quot = framestr_start_digital_char(p_colon);
										if(p_left_quot && p_left_quot < p_comma)
										{
											p_right_quot = framestr_end_digital_char(p_left_quot);
											if(p_right_quot && p_right_quot <= p_comma)
											{
												str_len = p_right_quot - p_left_quot;
												if(str_len > 0)
												{
													str_len++;
													node->value_= (char *)ngx_pnalloc(res->mem,str_len);
													if(node->value_ == NULL)
													{
														PERROR("system malloc error\n");
														return -1;
													}
													memset(node->value_,0,str_len);
													str_len--;
													memcpy(node->value_,p_left_quot,str_len);
												}
											}
										}
									}
								}
								p_char = p_comma + 1;
								add_keyvalue_after_head(obj_node->head_,node);
								node = NULL;
								if(is_obj_end == 1)
									break;
							}
							*bit_move = in_move + p_end_obj - str + 1;
							obj_node = NULL;
						}else{
							PDEBUG("error:: %s\n",p_left_brack);
						}
					}else{
						PDEBUG("error :: %s\n",p_left_brack);
						break;
					}
				}else{
					PDEBUG("error :: %s\n",p_left_brack);
					break;
				}

				q_char = p_end_obj + 1;
				p_left_brack = framestr_frist_constchar(q_char,123);//{{
			}

			PDEBUG("There is balance str:: %s can't analysis,need wait enought str\n",q_char);
			p_right_square = framestr_frist_constchar(q_char,93);//]]]
			if(p_right_square)
			{
				*bit_move = in_move + p_right_square - str + 1;
				return 0;
			}
			else
				return 1;
		}
	}else{
		
		if(res->http->ret < 200 || res->http->ret >= 300)
		{
			str_len = res->http->body_len + 1;
			res->err_message = (char *)ngx_pnalloc(res->mem,str_len);
			if(res->err_message  == NULL)
			{
				PERROR("system malloc error\n");
				return -1;
			}
			
			if(buf_len > res->http->body_len)
			{
				memcpy(res->err_message,q_char,res->http->body_len);
				*bit_move = in_move + res->http->body_len;
				return 0;
			}else{
				memcpy(res->err_message,q_char,buf_len);
				*bit_move = in_move + buf_len;
				return 1;
			}
		}

		if(res->http->body_len < 90)
		{
			str_len = res->http->body_len + 1;
			res->message = (char *)ngx_pnalloc(res->mem,str_len);
			if(res->message  == NULL)
			{
				PERROR("system malloc error\n");
				return -1;
			}

			if(buf_len > res->http->body_len)
			{
				memcpy(res->message,q_char,res->http->body_len);
				*bit_move = in_move + res->http->body_len;
				return 0;
			}else{
				memcpy(res->message,q_char,buf_len);
				*bit_move = in_move + buf_len;
				return 1;
			}
		}

		p_left_brack = framestr_frist_constchar(q_char,123);///{
		if(p_left_brack)
		{
			q_char = p_left_brack;
			p_char = framestr_first_conststr(q_char,hit_str);
			if(p_char)
			{
				q_char = p_char + frame_strlen(hit_str);
				p_char = framestr_first_conststr(q_char,total_str);
				if(p_char)
				{
					q_char = p_char + frame_strlen(total_str);
					p_colon = framestr_frist_constchar(q_char,58);//:::
					if(p_colon)
					{
						p_char = framestr_start_digital_char(p_colon);
						if(p_char)
						{
							q_char = framestr_end_digital_char(p_char);
							if(q_char)
							{
								res->search = (ES_SEARCH_RESULT *)ngx_pnalloc(res->mem,sizeof(ES_SEARCH_RESULT));
								if(res->search == NULL)
								{
									PERROR("system malloc error\n");
									return -1;
								}
								memset(res->search,0,sizeof(ES_SEARCH_RESULT));
								str_len = q_char - p_char;
								if(str_len > 0)
								{
									memset(tmp_char,0,32);
									memcpy(tmp_char,q_char,str_len);
									res->search->total = atoi(tmp_char);
								}else{
									PERROR("There is no found of total str value\n");
								}
								*bit_move = in_move + p_char - str;
							}else{
								PERROR("There is no found \"total\" value\n");
							}
						}else{
							PERROR("There is no found \"total\" value\n");
						}
					}else{
						PERROR("There is no found \"total\" value\n");
					}

					if(res->search)
					{
						p_char = framestr_first_conststr(q_char,hit_str);
						if(p_char)
						{
							q_char = p_char;
							{
								p_left_brack = framestr_frist_constchar(q_char,123);//{{
								while(p_left_brack)
								{
									p_char = p_left_brack;
									p_right_brack = framestr_frist_constchar(p_left_brack,125);//}}
									if(p_right_brack)
									{
										p_right_brack++;
										p_end_obj = framestr_frist_constchar(p_right_brack,125);//}}
										if(p_end_obj)
										{
											p_left_brack++;
											p_left_brack = framestr_frist_constchar(p_left_brack,123);//{{
											if(p_left_brack && p_left_brack < p_right_brack)
											{
												obj_node = (OBJ_INFO *)ngx_pnalloc(res->mem,sizeof(OBJ_INFO));
												if(obj_node == NULL)
												{
													PERROR("system malloc error\n");
													return -1;
												}
												
												p_char = framestr_first_conststr(p_char,_index_str);
												if(p_char && p_char < p_left_brack)
												{
													p_comma = framestr_frist_constchar(p_char,44);///,
													if(!p_comma || p_comma > p_left_brack)
														p_comma = p_left_brack;
													p_colon = framestr_frist_constchar(p_char,58);//:::
													if(p_colon && p_colon < p_comma)
													{
														p_left_quot = framestr_frist_constchar(p_colon,34);//"""
														if(!p_left_quot || p_left_quot > p_comma)
														{
															p_left_quot = framestr_start_digital_char(p_colon);
															if(p_left_quot && p_left_quot < p_comma)
															{
																p_right_quot = framestr_end_digital_char(p_left_quot);
																if(p_right_quot && p_right_quot <= p_comma)
																{
																	str_len = p_right_quot - p_left_quot;
																	if(str_len > 0)
																	{
																		str_len++;
																		obj_node->index = (char *)ngx_pnalloc(res->mem,str_len);
																		if(obj_node->index == NULL)
																		{
																			PERROR("system malloc error\n");
																			return -1;
																		}
																		memset(obj_node->index,0,str_len);
																		str_len--;
																		memcpy(obj_node->index,p_left_quot,str_len);
																	}
																}
															}
														}else{
															p_left_quot++;
															p_right_quot = framestr_frist_constchar(p_left_quot,34);//"""
															if(p_right_quot && p_right_quot < p_comma)
															{
																str_len = p_right_quot - p_left_quot;
																if(str_len > 0)
																{
																	str_len++;
																	obj_node->index = (char *)ngx_pnalloc(res->mem,str_len);
																	if(obj_node->index == NULL)
																	{
																		PERROR("system malloc error\n");
																		return -1;
																	}
																	memset(obj_node->index,0,str_len);
																	str_len--;
																	memcpy(obj_node->index,p_left_quot,str_len);
																}
															}
														}
													}

													p_char = p_comma + 1;
												}

												p_char = framestr_first_conststr(p_char,_type_str);
												if(p_char && p_char < p_left_brack)
												{
													p_comma = framestr_frist_constchar(p_char,44);///,
													if(!p_comma || p_comma > p_left_brack)
														p_comma = p_left_brack;
													p_colon = framestr_frist_constchar(p_char,58);//:::
													if(p_colon && p_colon < p_comma)
													{
														p_left_quot = framestr_frist_constchar(p_colon,34);//"""
														if(!p_left_quot || p_left_quot > p_comma)
														{
															p_left_quot = framestr_start_digital_char(p_colon);
															if(p_left_quot && p_left_quot < p_comma)
															{
																p_right_quot = framestr_end_digital_char(p_left_quot);
																if(p_right_quot && p_right_quot <= p_comma)
																{
																	str_len = p_right_quot - p_left_quot;
																	if(str_len > 0)
																	{
																		str_len++;
																		obj_node->type = (char *)ngx_pnalloc(res->mem,str_len);
																		if(obj_node->type == NULL)
																		{
																			PERROR("system malloc error\n");
																			return -1;
																		}
																		memset(obj_node->type,0,str_len);
																		str_len--;
																		memcpy(obj_node->type,p_left_quot,str_len);
																	}
																}
															}
														}else{
															p_left_quot++;
															p_right_quot = framestr_frist_constchar(p_left_quot,34);//"""
															if(p_right_quot && p_right_quot < p_comma)
															{
																str_len = p_right_quot - p_left_quot;
																if(str_len > 0)
																{
																	str_len++;
																	obj_node->type = (char *)ngx_pnalloc(res->mem,str_len);
																	if(obj_node->type == NULL)
																	{
																		PERROR("system malloc error\n");
																		return -1;
																	}
																	memset(obj_node->type,0,str_len);
																	str_len--;
																	memcpy(obj_node->type,p_left_quot,str_len);
																}
															}
														}
													}

													p_char = p_comma + 1;
												}

												p_char = framestr_first_conststr(p_char,_id_str);
												if(p_char && p_char < p_left_brack)
												{
													p_comma = framestr_frist_constchar(p_char,44);///,
													if(!p_comma || p_comma > p_left_brack)
														p_comma = p_left_brack;
													p_colon = framestr_frist_constchar(p_char,58);//:::
													if(p_colon && p_colon < p_comma)
													{
														p_left_quot = framestr_frist_constchar(p_colon,34);//"""
														if(!p_left_quot || p_left_quot > p_comma)
														{
															p_left_quot = framestr_start_digital_char(p_colon);
															if(p_left_quot && p_left_quot < p_comma)
															{
																p_right_quot = framestr_end_digital_char(p_left_quot);
																if(p_right_quot && p_right_quot <= p_comma)
																{
																	str_len = p_right_quot - p_left_quot;
																	if(str_len > 0)
																	{
																		str_len++;
																		obj_node->id_ = (char *)ngx_pnalloc(res->mem,str_len);
																		if(obj_node->id_ == NULL)
																		{
																			PERROR("system malloc error\n");
																			return -1;
																		}
																		memset(obj_node->id_,0,str_len);
																		str_len--;
																		memcpy(obj_node->id_,p_left_quot,str_len);
																	}
																}
															}
														}else{
															p_left_quot++;
															p_right_quot = framestr_frist_constchar(p_left_quot,34);//"""
															if(p_right_quot && p_right_quot < p_comma)
															{
																str_len = p_right_quot - p_left_quot;
																if(str_len > 0)
																{
																	str_len++;
																	obj_node->id_ = (char *)ngx_pnalloc(res->mem,str_len);
																	if(obj_node->id_ == NULL)
																	{
																		PERROR("system malloc error\n");
																		return -1;
																	}
																	memset(obj_node->id_,0,str_len);
																	str_len--;
																	memcpy(obj_node->id_,p_left_quot,str_len);
																}
															}
														}
													}

													p_char = p_comma + 1;
												}
												if(res->search->head == NULL)
												{
													add_objinfo_after_head(res->search->head,obj_node);
													res->search->tail = obj_node;
												}else{
													add_objinfo_after_tail(&(res->search->tail),obj_node);
												}
												
												p_char = p_left_brack;
												is_obj_end = 0;
												while(1)
												{
													p_comma = framestr_frist_constchar(p_char,44);///,
													if(!p_comma || p_comma > p_right_brack)
													{
														p_comma = p_right_brack;
														is_obj_end = 1;
													}

													p_colon = framestr_frist_constchar(p_char,58);//:::
													if(p_colon && p_colon < p_comma)
													{
														node = (KEY_VALUE_NODE *)ngx_pnalloc(res->mem,sizeof(KEY_VALUE_NODE));
														if(node == NULL)
														{
															PERROR("system malloc error\n");
															return -1;
														}
														memset(node,0,sizeof(KEY_VALUE_NODE));
														
														p_left_quot = framestr_frist_constchar(p_char,34);///,
														if(p_left_quot && p_left_quot < p_colon)
														{
															p_left_quot++;
															p_right_quot = framestr_frist_constchar(p_left_quot,34);///,
															if(p_right_quot && p_right_quot < p_colon)
															{
																str_len = p_right_quot - p_left_quot;
																if(str_len > 0)
																{
																	str_len++;
																	node->key_ = (char *)ngx_pnalloc(res->mem,str_len);
																	if(node->key_ == NULL)
																	{
																		PERROR("system malloc error\n");
																		return -1;
																	}
																	memset(node->key_,0,str_len);
																	str_len--;
																	memcpy(node->key_,p_left_quot,str_len);
																}
															}
														}

														p_left_quot = framestr_frist_constchar(p_colon,34);///,
														if(p_left_quot && p_left_quot < p_comma)
														{
															p_left_quot++;
															p_right_quot = framestr_frist_constchar(p_left_quot,34);///,
															if(p_right_quot && p_right_quot < p_comma)
															{
																str_len = p_right_quot - p_left_quot;
																if(str_len > 0)
																{
																	str_len++;
																	node->value_ = (char *)ngx_pnalloc(res->mem,str_len);
																	if(node->value_ == NULL)
																	{
																		PERROR("system malloc error\n");
																		return -1;
																	}
																	memset(node->value_,0,str_len);
																	str_len--;
																	memcpy(node->value_,p_left_quot,str_len);
																}
															}
														}else{
															p_left_quot = framestr_start_digital_char(p_colon);
															if(p_left_quot && p_left_quot < p_comma)
															{
																p_right_quot = framestr_end_digital_char(p_left_quot);
																if(p_right_quot && p_right_quot <= p_comma)
																{
																	str_len = p_right_quot - p_left_quot;
																	if(str_len > 0)
																	{
																		str_len++;
																		node->value_= (char *)ngx_pnalloc(res->mem,str_len);
																		if(node->value_ == NULL)
																		{
																			PERROR("system malloc error\n");
																			return -1;
																		}
																		memset(node->value_,0,str_len);
																		str_len--;
																		memcpy(node->value_,p_left_quot,str_len);
																	}
																}
															}
														}
													}
													p_char = p_comma + 1;
													add_keyvalue_after_head(obj_node->head_,node);
													node = NULL;
													if(is_obj_end == 1)
														break;
												}
												*bit_move = in_move + p_end_obj - str + 1;
												obj_node = NULL;
											}else{
												PDEBUG("error:: %s\n",p_left_brack);
											}
										}else{
											PDEBUG("error :: %s\n",p_left_brack);
											break;
										}
									}else{
										PDEBUG("error :: %s\n",p_left_brack);
										break;
									}

									q_char = p_end_obj + 1;
									p_left_brack = framestr_frist_constchar(q_char,123);//{{
								}

								PDEBUG("There is balance str:: %s can't analysis,need wait enought str\n",q_char);
								p_right_square = framestr_frist_constchar(q_char,93);//]]]
								if(p_right_square)
								{
									*bit_move = in_move + p_right_square - str + 1;
									return 0;
								}
								else
									return 1;
							}
						}
					}
					
				}else{
					PERROR("There is no found \"total\" string\n");
				}
			}else{
				PERROR("There is no found \"hits\" string\n");
			}
		}

		if(buf_len >= SOCKET_BUFF_LEN)
		{
			PERROR("message buf :: %s\n",p_char);
			str_len = res->http->body_len + 1;
			res->message = (char *)ngx_pnalloc(res->mem,str_len);
			if(res->message  == NULL)
			{
				PERROR("system malloc error\n");
				return -1;
			}

			if(buf_len > res->http->body_len)
			{
				memcpy(res->message,q_char,res->http->body_len);
				*bit_move = in_move + res->http->body_len;
				return 0;
			}else{
				memcpy(res->message,q_char,buf_len);
				*bit_move = in_move + buf_len;
				return 1;
			}
		}
		
		return 1;
	}
	
error_out:
	p_char = framestr_first_conststr(p_char,http_str);
	if(p_char)
	{
		*bit_move += p_char - str; 
	}else{
		*bit_move += buf_len;
	}
	return ret;
}

///return = 0 mean one respond is over,and will be deal another respond
///return > 0 
// 1 mean there is no over one message
///return < 0 mean there is something wrong about analysis str buf
// -1 mean malloc error
// -2 mean buf format error
///bit_mover means must give up how many byte;
int analysis_es_recv_str(char *buf,ES_RESPOND *res,int* bit_move)
{
	int ret = 0;
	char http_str[] = "HTTP",*p_char = buf,*q_char = buf;

	*bit_move = 0;
	p_char = framestr_first_conststr(p_char,http_str);
	if(p_char)//begin flag or new recv mess
	{
		if(res->http == NULL)
		{
			PDEBUG("analysis http head first\n");
			ret = analysis_http_head_str(p_char,res,bit_move);
			if(ret < 0)
				return ret;
			q_char += *bit_move;
			////////////////////////////////////////head is end
			return analysis_http_json_str(q_char,res,bit_move);
		}else{
			PDEBUG("analysis http body first\n");
			return analysis_http_json_str(q_char,res,bit_move);
		}
	}else{/// no any begin flag,there data just data;
		PDEBUG("analysis http body\n");
		return analysis_http_json_str(q_char,res,bit_move);
	}
}

void inside_release_es_respond(ES_RESPOND* res)
{
	/*
	if(res->sta_ != 3)
	{
		PERROR("never been here\n");
	}*/
	res->sta_ = 0;
	res->obj_num_  = 0;

	res->http = NULL;
	res->message = NULL;
	res->err_message = NULL;
	res->search = NULL;
	
	res->req_ = NULL;
	res->req_buf = NULL;
	res->call_ = NULL;
	if(es_info.is_run == 0)
	{
		ngx_destroy_pool(res->mem);
	}else{
		ngx_reset_pool(res->mem);
		sem_post(&es_info.free_num);
	}
}

void es_server_send(void *data)
{
	int ret = 0,index = 0;
	ES_RESPOND *p_res = NULL;
	PDEBUG("es_server_send start......\n");
	while(1)
	{
		index = get_send_index();
		if(index < 0)
			break;
		p_res = es_res_array + index;
		if(p_res->sta_ != 1)
		{
			PERROR("never been here\n");
		}
		ret = tcp_client_send(es_info.es_fd_,p_res->req_buf);
		if(ret == 0)
		{
			ret = es_server_connect(es_info.es_fd_);
			if(ret < 0)
			{
				break;
			}else{
				continue;
			}
		}else{
			sem_post(&es_info.recv_num);
		}

		p_res->sta_ = 2;
	}

	shutdown(es_info.es_fd_,SHUT_WR);//SHUT_RD;SHUT_RDWR
}

void es_server_recv(void *data)
{
	int ret = 0,index = 0,bit_move = 0;
	char an_buf[AN_SOCKET_BUFF_LEN] = {0},r_buf[SOCKET_BUFF_LEN] = {0},*p_recv = an_buf,*p_char = NULL;
	ES_RESPOND *p_res = NULL;
	PDEBUG("es_server_recv start......\n");
	while(1)
	{
		if(ret == 0)
		{
			if(p_res)
			{
				p_res->sta_ = 3;
				sem_post(&es_info.call_num);
			}
			index = get_recv_index();
			PDEBUG("index :: %d\n",index);
			if(index < 0)
				break;
			p_res = es_res_array + index;
			if(p_res->sta_ != 2)
			{
				PERROR("never been here\n");
			}
		}
		
		memset(r_buf,0,SOCKET_BUFF_LEN);
		ret = tcp_client_recv(es_info.es_fd_,r_buf,SOCKET_BUF_RECV);
		PDEBUG("tcp_client_recv :: ret :: %d\n",ret);
		if(ret <= 0)
		{
			ret = es_server_connect(es_info.es_fd_);
			if(ret < 0)
			{
				break;
			}else{
				continue;
			}
		}else if(ret <= SOCKET_BUF_RECV)
		{
			p_char = frame_end_charstr(an_buf);
			memcpy(p_char,r_buf,ret);
			ret = analysis_es_recv_str(p_recv,p_res,&bit_move);
			PDEBUG("analysis_es_recv_str :: ret :: %d\n",ret);
			memory_char_move_bit(an_buf,bit_move,AN_SOCKET_BUFF_LEN);
		}else if(ret > SOCKET_BUF_RECV)
		{
			continue;
		}else{//may be bug,TODO// never be here
			p_char = frame_end_charstr(an_buf);
			memcpy(p_char,r_buf,ret);
			ret = analysis_es_recv_str(p_recv,p_res,&bit_move);
			PDEBUG("analysis_es_recv_str :: ret :: %d\n",ret);
			memory_char_move_bit(an_buf,bit_move,AN_SOCKET_BUFF_LEN);
		}
		
		if(ret < 0)
		{
			PERROR("There is something wrong to analysis buf\n");
			if(ret == -1)
			{
				es_info.is_run = 0;
				break;
			}
		}
		
	}

	shutdown(es_info.es_fd_,SHUT_RD);//SHUT_RD;SHUT_RDWR
}

void es_asynchronous_callback(void* data)
{
	int index = 0;
	ES_RESPOND *p_res = NULL;
	PDEBUG("es_asynchronous_callback start......\n");
	while(1)
	{
		index = get_call_index();
		if(index < 0)
			break;
		p_res = es_res_array + index;
		if(p_res->sta_ != 3)
		{
			PERROR("never been here\n");
		}
		if(p_res->call_)
		{
			p_res->call_(p_res);
			p_res->sta_ = 0;
		}
		inside_release_es_respond(p_res);
		p_res = NULL;
	}
	
	close(es_info.es_fd_);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
int es_server_init(const char *ip_str,unsigned short port,PROTOCOL_TYPE type)
{
	int ret = 0,i = 0;
	ES_RESPOND *p_res = es_res_array;
	memset(&es_info,0,sizeof(ES_SERVER_INFO));

	if(ip_str == NULL)
		return -1;
	es_info.type_ = type;
	es_info.es_fd_ = socket_tcp_ipv4();
	if(es_info.es_fd_ < 0)
		return es_info.es_fd_;
	es_info.es_fd_b = socket_tcp_ipv4();
	if(es_info.es_fd_b < 0)
		return es_info.es_fd_b;
	es_info.is_run = 1;
	memcpy(es_info.ip_addr,ip_str,frame_strlen(ip_str));
	es_info.port_ = port;
	
	ret = es_server_connect(es_info.es_fd_);
	if(ret < 0)
		return ret;

	ret = es_server_connect(es_info.es_fd_b);
	if(ret < 0)
		return ret;

	sem_init(&es_info.free_num, 0, ES_RESPOND_ARRAY_LEN);
	sem_init(&es_info.send_num, 0, 0);
	sem_init(&es_info.recv_num, 0, 0);
	sem_init(&es_info.call_num, 0, 0);

	pthread_mutex_init(&es_info.asyn_mutex,NULL);
	pthread_mutex_init(&es_info.bloc_mutex,NULL);
	
	for(i = 0;i < ES_RESPOND_ARRAY_ALL_LEN;i++)
	{
		p_res->mem = ngx_create_pool(ES_MEM_POLL_SIZE);
		if(p_res->mem == NULL)
			return -1;
		p_res++;
	}
}

void release_es_respond()
{
	ES_RESPOND* res = NULL;
	res = es_res_array + ES_RESPOND_ARRAY_LEN;
	
	if(res->sta_ != 2)
	{
		PERROR("never been here\n");
	}
	res->sta_ = 0;
	res->obj_num_  = 0;

	res->http = NULL;
	res->err_message = NULL;
	res->message = NULL;
	res->search = NULL;
	
	res->req_ = NULL;
	res->req_buf = NULL;
	res->call_ = NULL;
	if(es_info.is_run == 0)
	{
		ngx_destroy_pool(res->mem);
	}else{
		ngx_reset_pool(res->mem);
	}
	pthread_mutex_unlock(&es_info.bloc_mutex);
}

ES_RESPOND* es_query_block(ES_REQUEST *req)
{
	int str_len = 0,i = 0,ret = 0;
	char path_str[SOCKET_BUFF_LEN] = {0},recv_buf[SOCKET_BUFF_LEN] = {0};
	MAKE_HTTP_MESSAGE par = {0};
	ES_RESPOND *p_res = NULL;

	if(req->path_str == NULL)
		return NULL;
	if(es_info.is_run == 0)
		return NULL;
	pthread_mutex_lock(&es_info.bloc_mutex);
	p_res = es_res_array + ES_RESPOND_ARRAY_LEN;
	
	if(req->path_str && req->query_str)
		sprintf(path_str,"%s?%s",req->path_str,req->query_str);
	else
		sprintf(path_str,"%s",req->path_str);
	par.h_head = verb_str_arr[req->verb];
	par.path = path_str;
	if(req->body_str)
	{
		par.body = req->body_str;
	}
	
	par.ip_str = es_info.ip_addr;
	par.port = es_info.port_;
	do{
		i++;
		str_len = SOCKET_BUFF_LEN * i;
		p_res->req_buf = (char*)ngx_pnalloc(p_res->mem,str_len);
		if(p_res->req_buf == NULL)
		{
			goto error_out;
		}
		par.h_buf = p_res->req_buf;
		par.h_buf_len = str_len;
		ret = make_es_http_message(&par);
		if(ret < str_len)
		{
			break;
		}
	}while(1);

send:
	ret = tcp_client_send(es_info.es_fd_b,p_res->req_buf);
	if(ret == 0)
	{
		ret = es_server_connect(es_info.es_fd_b);
		if(ret < 0)
		{
			goto error_out;
		}else{
			goto send;
		}
	}
recv:
	ret = tcp_client_recv(es_info.es_fd_b,recv_buf,SOCKET_BUF_RECV);
	if(ret <= 0)
	{
		ret = es_server_connect(es_info.es_fd_b);
		if(ret < 0)
		{
			goto error_out;
		}else{
			goto recv;
		}
	}else if(ret > SOCKET_BUF_RECV)
	{
		goto recv;
	}
	printf("%s",recv_buf);
	if(ret == SOCKET_BUF_RECV)
		goto recv;
	
	return p_res;
error_out:
	
	return NULL;
}

int es_query_asynchronous(ES_REQUEST *req,RESPOND_CALLBACL call)
{
	ES_RESPOND *p_res = NULL;
	int str_len = 0,index = 0,i = 0,ret = 0;
	MAKE_HTTP_MESSAGE par = {0};
	char path_str[SOCKET_BUFF_LEN] = {0},*p_char = NULL;

	if(req->path_str == NULL)
		return -3;
	
	if(es_info.is_run == 0)
		return -2;
	pthread_mutex_lock(&es_info.asyn_mutex);
	index = get_insert_index();
	p_res = es_res_array + index;
	if(p_res->sta_ != 0)
	{
		PERROR("never been here\n");
	}

	p_res->call_ = call;
	p_res->req_ = (ES_REQUEST*)ngx_pnalloc(p_res->mem,sizeof(ES_REQUEST));
	if(p_res->req_ == NULL)
	{
		ret = -1;
		goto error_out;
	}
	p_res->req_->verb = req->verb;
	
	if(req->path_str)
	{
		str_len = frame_strlen(req->path_str) + 1;
		p_char = (char*)ngx_pnalloc(p_res->mem,str_len);
		if(p_char == NULL)
		{
			ret = -1;
			goto error_out;
		}
		memset(p_char,0,str_len);
		str_len--;
		memcpy(p_char,req->path_str,str_len);
		p_res->req_->path_str = p_char;
	}
	
	if(req->query_str)
	{
		str_len = frame_strlen(req->query_str) + 1;
		p_char = (char*)ngx_pnalloc(p_res->mem,str_len);
		if(p_char == NULL)
		{
			ret = -1;
			goto error_out;
		}
		memset(p_char,0,str_len);
		str_len--;
		memcpy(p_char,req->query_str,str_len);
		p_res->req_->query_str = p_char;
	}

	if(req->body_str)
	{
		str_len = frame_strlen(req->body_str) + 1;
		p_char = (char*)ngx_pnalloc(p_res->mem,str_len);
		if(p_char == NULL)
		{
			ret = -1;
			goto error_out;
		}
		memset(p_char,0,str_len);
		str_len--;
		memcpy(p_char,req->body_str,str_len);
		p_res->req_->body_str = p_char;
	}
	
	if(req->path_str && req->query_str)
		sprintf(path_str,"%s%s",req->path_str,req->query_str);
	else
		sprintf(path_str,"%s",req->path_str);
	par.h_head = verb_str_arr[req->verb];
	par.path = path_str;
	if(req->body_str)
	{
		par.body = req->body_str;
	}
	
	par.ip_str = es_info.ip_addr;
	par.port = es_info.port_;
	do{
		i++;
		str_len = SOCKET_BUFF_LEN * i;
		p_res->req_buf = (char*)ngx_pnalloc(p_res->mem,str_len);
		if(p_res->req_buf == NULL)
		{
			ret = -1;
			goto error_out;
		}
		par.h_buf = p_res->req_buf;
		par.h_buf_len = str_len;
		ret = make_es_http_message(&par);
		if(ret < str_len)
		{
			break;
		}
	}while(1);

	p_res->sta_ = 1;
	sem_post(&es_info.send_num);
	pthread_mutex_unlock(&es_info.asyn_mutex);
	return 0;
error_out:	
	p_res->sta_ = 0;
	p_res->obj_num_  = 0;

	p_res->http = NULL;
	p_res->err_message = NULL;
	p_res->message = NULL;
	p_res->search = NULL;
	
	p_res->req_ = NULL;
	p_res->req_buf = NULL;
	p_res->call_ = NULL;
	ngx_reset_pool(p_res->mem);
	
	es_info.insert_index--;
	sem_post(&es_info.free_num);
	pthread_mutex_unlock(&es_info.asyn_mutex);
	return ret;
}

void es_server_destroy()
{
	int i = 0;
	ES_RESPOND* res = es_res_array;
	es_info.is_run = 0;
	/*
	if(es_info.es_fd_)
		close(es_info.es_fd_);
	if(es_info.es_fd_b)
		close(es_info.es_fd_b);*/
	//memset(&es_info,0,sizeof(ES_SERVER_INFO));
	
	for(i = 0;i < ES_RESPOND_ARRAY_ALL_LEN;i++)
	{
		if(res->sta_ == 0)
			ngx_destroy_pool(res->mem);
		res++;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void es_asynchronous_test_functon(ES_RESPOND *ret)
{
	KEY_VALUE_NODE *p = NULL;
	OBJ_INFO *q = NULL;
	
	if(ret == NULL)
	{
		PERROR("The result is nuLL point\n");
		return;
	}

	PDEBUG("ret->sta_ :: %d \n",ret->sta_);
	PDEBUG("ret->obj_num_ :: %d \n",ret->obj_num_);

	if(ret->http == NULL)
	{
		PERROR("The ES query http head is null\n");
		return;
	}else{
		PDEBUG("ret->http->ver :: %s \n",ret->http->ver);
		PDEBUG("ret->http->note :: %s \n",ret->http->note);
		PDEBUG("ret->http->ret :: %d \n",ret->http->ret);
		PDEBUG("ret->http->body_len :: %d \n",ret->http->body_len);

		if(ret->http->head_ == NULL)
		{
			PERROR("The ES query http head key value is null \n");
		}else{
			for(p = ret->http->head_;p != NULL;p = p->next_)
			{
				PDEBUG("ret->http->head_:: %s : %s\n",p->key_,p->value_);
			}
		}
	}

	if(ret->message)
	{
		PDEBUG("ret->message :: %s\n",ret->message);
	}

	
	if(ret->err_message)
	{
		PDEBUG("ret->err_message :: %s\n",ret->err_message);
	}

	if(ret->search)
	{
		PDEBUG("ret->search->total :: %d\n",ret->search->total);
		if(ret->search->head == NULL)
		{
			PERROR("ret->search->head object is null\n");
		}else{
			for(q = ret->search->head;q != NULL;q = q->next_)
			{
				PDEBUG("ret->search->head->index :: %s \n",q->index);
				PDEBUG("ret->search->head->type :: %s \n",q->type);
				PDEBUG("ret->search->head->id_ :: %s \n",q->id_);
				if(q->head_ == NULL)
				{
					PERROR("The object search is null\n");
				}else{
					for(p = q->head_ ; p != NULL;p =p->next_)
					{
						PDEBUG("ret->search->head key value:: %s : %s\n",p->key_,p->value_);
					}
				}
			}
		}
	}
}

void es_server_query(VERB_METHOD verb,const char *path_str,const char *query_str,const char *body_str)
{
	int body_len = 0,ret = 0;
	char buff[SOCKET_BUFF_LEN] = {0},r_buf[SOCKET_BUFF_LEN] = {0},*p_char = r_buf + SOCKET_BUFF_LEN;

	if(path_str == NULL)
		return;
	if(query_str && body_str)
	{
		body_len = frame_strlen(body_str);
		//text/html;
		sprintf(buff,"%s %s?%s %s\r\n\
User-Agent: ES-HTTP-API\r\n\
Content-type: application/json;charset=UTF-8\r\n\
Host: %s:%d\r\n\
Accept: text/html, application/json, image/gif, image/jpeg, *; q=.2, *//*; q=.2\r\n\
Connection: keep-alive\r\n\
Content-Length: %d\r\n\r\n\
%s",verb_str_arr[verb],path_str,query_str,protocol_str[es_info.type_],es_info.ip_addr,es_info.port_,body_len,body_str);
	}else if(query_str)
	{
		sprintf(buff,"%s %s?%s %s\r\n\
User-Agent: ES-HTTP-API\r\n\
Content-type: application/json;charset=UTF-8\r\n\
Host: %s:%d\r\n\
Accept: text/html, application/json, image/gif, image/jpeg, *; q=.2, *//*; q=.2\r\n\
Connection: keep-alive\r\n\
Content-Length: %d\r\n\r\n",verb_str_arr[verb],path_str,query_str,protocol_str[es_info.type_],es_info.ip_addr,es_info.port_,body_len);
	}else if(body_str)
	{
		body_len = frame_strlen(body_str);
		//text/html;
		sprintf(buff,"%s %s %s\r\n\
User-Agent: ES-HTTP-API\r\n\
Content-type: application/json;charset=UTF-8\r\n\
Host: %s:%d\r\n\
Accept: text/html, application/json, image/gif, image/jpeg, *; q=.2, *//*; q=.2\r\n\
Connection: keep-alive\r\n\
Content-Length: %d\r\n\r\n\
%s",verb_str_arr[verb],path_str,protocol_str[es_info.type_],es_info.ip_addr,es_info.port_,body_len,body_str);
	}else{
		sprintf(buff,"%s %s %s\r\n\
User-Agent: ES-HTTP-API\r\n\
Content-type: application/json;charset=UTF-8\r\n\
Host: %s:%d\r\n\
Accept: text/html, application/json, image/gif, image/jpeg, *; q=.2, *//*; q=.2\r\n\
Connection: keep-alive\r\n\
Content-Length: %d\r\n\r\n",verb_str_arr[verb],path_str,protocol_str[es_info.type_],es_info.ip_addr,es_info.port_,body_len);
	}
	//printf("%s\n\n",buff);
	body_len = frame_strlen(buff);
send:
	ret = tcp_client_send(es_info.es_fd_,buff);
	if(ret == 0)
	{
		ret = es_server_connect(es_info.es_fd_);
		if(ret < 0)
		{
			return;
		}else{
			goto send;
		}
	}
recv:
	ret = tcp_client_recv(es_info.es_fd_,r_buf,SOCKET_BUF_RECV);
	if(ret <= 0)
	{
		ret = es_server_connect(es_info.es_fd_);
		if(ret < 0)
		{
			return;
		}else{
			goto recv;
		}
	}else if(ret > SOCKET_BUF_RECV)
	{
		goto recv;
	}
	printf("%s",r_buf);
	if(ret == SOCKET_BUF_RECV)
		goto recv;
}


