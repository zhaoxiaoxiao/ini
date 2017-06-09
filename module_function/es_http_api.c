
#include <pthread.h>
#include <semaphore.h>

#include "common.h"
#include "es_http_api.h"
#include "socket_api.h"
#include "frame_tool.h"
#include "memory_pool.h"

#define SOCKET_BUFF_LEN			(1024)
#define SOCKET_BUF_RECV			(SOCKET_BUFF_LEN - 1)

#define ES_RESPOND_ARRAY_LEN	(10)

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
	int es_fd_asy;

	int req_index,res_index,insert_req_sub;
	sem_t free_num,active_num;
	
	PROTOCOL_TYPE type_;
	unsigned short port_;

	
}ES_SERVER_INFO;

static ES_SERVER_INFO es_info = {0};
static ES_RESPOND es_res_array[ES_RESPOND_ARRAY_LEN] = {0};

////////////////////////////////////////////////////////////////////////////////////////////////////////
int es_server_connect()
{
	return connect_tcp_server(es_info.es_fd_,es_info.port_,es_info.ip_addr);
}

int get_req_index()
{
	int index = 0;
	sem_wait(&es_info.active_num);
	index = es_info.req_index;
	es_info.req_index++;
	if(es_info.req_index >= ES_RESPOND_ARRAY_LEN)
		es_info.req_index = 0;
	return index;
}

int get_insert_req_index()
{
	int index = 0;
	sem_wait(&es_info.free_num);
	index = es_info.insert_req_sub;
	es_info.insert_req_sub++;
	if(es_info.insert_req_sub >= ES_RESPOND_ARRAY_LEN)
		es_info.insert_req_sub = 0;
	return index;
}

void es_server_send(void *data)
{
	int ret = 0,req_index = 0;
	ES_RESPOND *p_res = NULL;

	while(1)
	{
		req_index = get_req_index();
		p_res = es_res_array + req_index;
		ret = tcp_client_send(es_info.es_fd_,p_res->req_buf);
		if(ret == 0)
		{
			ret = es_server_connect();
			if(ret < 0)
			{
				break;
			}else{
				continue;
			}
		}
		
	}
}

void es_server_recv(void *data)
{
	int ret = 0,is_end = 0;
	char r_buf[SOCKET_BUFF_LEN] = {0};
	ES_RESPOND *p_res = NULL;

	while(1)
	{
		if(is_end == 0)
		{
			is_end = 1;
			if(es_info.res_index > es_info.req_index)
			{
				PERROR();
				
			}
			p_res = es_res_array + es_info.res_index;
			
		}
		ret = tcp_client_recv(es_info.es_fd_,r_buf,SOCKET_BUF_RECV);
		if(ret <= 0)
		{
			ret = es_server_connect();
			if(ret < 0)
			{
				break;
			}else{
				continue;
			}
		}
		printf("%s",r_buf);
		
		if(ret < SOCKET_BUF_RECV)
		{
			is_end = 0;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
int es_server_init(const char *ip_str,unsigned short port,PROTOCOL_TYPE type)
{
	int ret = 0,i = 0;
	
	memset(&es_info,0,sizeof(ES_SERVER_INFO));

	if(ip_str == NULL)
		return -1;
	es_info.type_ = type;
	es_info.es_fd_ = socket_tcp_ipv4();
	if(es_info.es_fd_ < 0)
		return es_info.es_fd_;
	memcpy(es_info.ip_addr,ip_str,frame_strlen(ip_str));
	es_info.port_ = port;
	
	ret = es_server_connect();
	if(ret < 0)
		return ret;

	for(i = 0;i < ES_RESPOND_ARRAY_LEN;i++)
	{
		
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
		ret = es_server_connect();
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
		ret = es_server_connect();
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

void es_query_asynchronous(ES_REQUEST *req,RESPOND_CALLBACL call)
{
}

void es_server_destroy()
{
	close(es_info.es_fd_);
	
	memset(&es_info,0,sizeof(ES_SERVER_INFO));
}

