
#include "common.h"
#include "http_applayer.h"
#include "frame_tool.h"


int make_http_post_json(char *h_buf,int len,char *path,char *body_str)
{
	int h_buf_len = 0;
	h_buf_len = frame_strlen(body_str);
	h_buf_len = snprintf(h_buf,len,"POST %s HTTP/1.1\r\n\
User-Agent: ES-HTTP-API\r\n\
Content-type: application/json;charset=UTF-8\r\n\
Host: 127.0.0.0:80\r\n\
Accept: application/json, image/gif, image/jpeg, *; q=.2, *//*; q=.2\r\n\
Connection: keep-alive\r\n\
Content-Length: %d\r\n\r\n\
%s",path,h_buf_len,body_str);

	return h_buf_len;
}

int make_http_post_text(char *h_buf,int len,char *path,char *body_str)
{
	int h_buf_len = 0;
	h_buf_len = frame_strlen(body_str);
	h_buf_len = snprintf(h_buf,len,"POST %s HTTP/1.1\r\n\
User-Agent: ES-HTTP-API\r\n\
Content-type: application/json;charset=UTF-8\r\n\
Host: 127.0.0.0:80\r\n\
Accept: text/html, image/gif, image/jpeg, *; q=.2, *//*; q=.2\r\n\
Connection: keep-alive\r\n\
Content-Length: %d\r\n\r\n\
%s",path,h_buf_len,body_str);

	return h_buf_len;
}

int make_http_get_test(char *h_buf,int len,char *path)
{
	int h_buf_len = 0;
	
	h_buf_len = snprintf(h_buf,len,"GET %s HTTP/1.1\r\n\
Host: 127.0.0.0:80\r\n\
Connection: keep-alive\r\n\
Upgrade-Insecure-Requests: 1\
User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/59.0.3071.86 Safari/537.36\
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\
Accept-Encoding: gzip, deflate\
Accept-Language: zh-CN,zh;q=0.8",path);

	return h_buf_len;
}


int make_es_http_message(MAKE_HTTP_MESSAGE *param)
{
	int ret = 0;

	if(param->h_head == NULL || param->h_buf == NULL || param->path == NULL || param->ip_str == NULL)
		return -1;

	if(param->h_buf_len <= 0 || param->port == 0)
		return -1;

	if(param->body)
	{
		ret = frame_strlen(param->body);
		ret = snprintf(param->h_buf,param->h_buf_len,"%s %s HTTP/1.1\r\n\
User-Agent: ES-HTTP-API\r\n\
Content-type: application/json;charset=UTF-8\r\n\
Host: %s:%d\r\n\
Accept: application/json; charset=UTF-8\r\n\
Connection: keep-alive\r\n\
Content-Length: %d\r\n\r\n\
%s",param->h_head,param->path,param->ip_str,param->port,ret,param->body);
	}else{
		ret = snprintf(param->h_buf,param->h_buf_len,"%s %s HTTP/1.1\r\n\
User-Agent: ES-HTTP-API\r\n\
Content-type: application/json;charset=UTF-8\r\n\
Host: %s:%d\r\n\
Accept: application/json; charset=UTF-8\r\n\
Connection: keep-alive\r\n\
Content-Length: %0\r\n\r\n",param->h_head,param->path,param->ip_str,param->port);
	}
	
	return ret;
}

