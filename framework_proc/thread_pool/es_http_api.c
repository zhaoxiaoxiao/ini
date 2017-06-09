
#include "common.h"
#include "es_http_api.h"
#include "socket_api.h"
#include "frame_tool.h"

#define SOCKET_BUFF_LEN		(1024)
#define SOCKET_BUF_RECV		(SOCKET_BUFF_LEN - 1)

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
	
	PROTOCOL_TYPE type_;
	unsigned short port_;

}ES_SERVER_INFO;

static ES_SERVER_INFO es_info = {0};

int es_server_connect()
{
	return connect_tcp_server(es_info.es_fd_,es_info.port_,es_info.ip_addr);
}

int es_server_init(const char *ip_str,unsigned short port,PROTOCOL_TYPE type)
{
	int ret = 0;
	
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


void es_server_destroy()
{
	close(es_info.es_fd_);
	
	memset(&es_info,0,sizeof(ES_SERVER_INFO));
}

