
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>

#include "common.h"
#include "memory_pool.h"
#include "key_value_part.h"
#include "es_obj_info.h"
#include "es_http_api.h"
#include "socket_api.h"
#include "frame_tool.h"
#include "http_applayer.h"


#define ES_MEM_POLL_SIZE			(1536)

#define SOCKET_BUFF_LEN				(1024)
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

void inside_release_es_respond(ES_RESPOND* res)
{
	/*
	if(res->sta_ != 3)
	{
		PERROR("never been here\n");
	}*/
	res->sta_ = 0;
	res->res_sta_ = 0;
	res->num_ = 0;
	res->head = NULL;
	res->tail = NULL;
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
	int ret = 0,is_end = 0,index = 0;
	char r_buf[SOCKET_BUFF_LEN] = {0};
	ES_RESPOND *p_res = NULL;

	while(1)
	{
		#if 0
		if(is_end == 0)
		{
			is_end = 1;
			index = get_recv_index();
			if(index < 0)
				break;
			p_res = es_res_array + index;
			if(p_res->sta_ != 2)
			{
				PERROR("never been here\n");
			}
		}
		#endif
		ret = tcp_client_recv(es_info.es_fd_,r_buf,SOCKET_BUF_RECV);
		if(ret <= 0)
		{
			ret = es_server_connect(es_info.es_fd_);
			if(ret < 0)
			{
				break;
			}else{
				continue;
			}
		}else if(ret == SOCKET_BUF_RECV)
		{
		}else if(ret > SOCKET_BUF_RECV)
		{
			continue;
		}else{//may be bug,TODO
			is_end = 0;
			p_res->sta_ = 3;
			sem_post(&es_info.call_num);
		}
		
		printf("%s",r_buf);
	}

	shutdown(es_info.es_fd_,SHUT_RD);//SHUT_RD;SHUT_RDWR
}

void es_asynchronous_callback(void* data)
{
	int index = 0;
	ES_RESPOND *p_res = NULL;
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
	res->res_sta_ = 0;
	res->num_ = 0;
	res->head = NULL;
	res->tail = NULL;
	res->req_ = NULL;
	res->req_buf = NULL;
	res->call_ = NULL;
	ngx_reset_pool(res->mem);
	pthread_mutex_unlock(&es_info.bloc_mutex);
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
		ret++;
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

	str_len = frame_strlen(req->path_str) + 1;
	p_char = (char*)ngx_pnalloc(p_res->mem,sizeof(str_len));
	if(p_char == NULL)
	{
		ret = -1;
		goto error_out;
	}
	memcpy(p_char,req->path_str,str_len);
	p_res->req_->path_str = p_char;

	str_len = frame_strlen(req->query_str) + 1;
	p_char = (char*)ngx_pnalloc(p_res->mem,sizeof(str_len));
	if(p_char == NULL)
	{
		ret = -1;
		goto error_out;
	}
	memcpy(p_char,req->query_str,str_len);
	p_res->req_->query_str = p_char;

	str_len = frame_strlen(req->body_str) + 1;
	p_char = (char*)ngx_pnalloc(p_res->mem,sizeof(str_len));
	if(p_char == NULL)
	{
		ret = -1;
		goto error_out;
	}
	memcpy(p_char,req->body_str,str_len);
	p_res->req_->body_str = p_char;
	
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
			ret = -1;
			goto error_out;
		}
		par.h_buf = p_res->req_buf;
		par.h_buf_len = str_len;
		ret = make_es_http_message(&par);
		ret++;
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
	p_res->res_sta_ = 0;
	p_res->num_ = 0;
	p_res->head = NULL;
	p_res->tail = NULL;
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

