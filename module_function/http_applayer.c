
#include "common.h"
#include "http_applayer.h"

int make_http_post_json(char *h_buf,int len,char *path,char *body_str)
{
	int h_buf_len = 0;
	h_buf_len = frame_strlen(body_str);
	snprintf(h_buf,len,"POST %s HTTP/1.1\r\n\
User-Agent: ES-HTTP-API\r\n\
Content-type: application/json;charset=UTF-8\r\n\
Host: %s:%d\r\n\
Accept: application/json, image/gif, image/jpeg, *; q=.2, *//*; q=.2\r\n\
Connection: keep-alive\r\n\
Content-Length: %d\r\n\r\n\
%s",path,es_info.ip_addr,es_info.port_,h_buf_len,body_str);

	h_buf_len = frame_strlen(h_buf);
	return h_buf_len;
}

int make_http_post_text(char *h_buf,int len,char *path,char *body_str)
{
	int h_buf_len = 0;
	h_buf_len = frame_strlen(body_str);
	snprintf(h_buf,len,"POST %s HTTP/1.1\r\n\
User-Agent: ES-HTTP-API\r\n\
Content-type: application/json;charset=UTF-8\r\n\
Host: %s:%d\r\n\
Accept: text/html, image/gif, image/jpeg, *; q=.2, *//*; q=.2\r\n\
Connection: keep-alive\r\n\
Content-Length: %d\r\n\r\n\
%s",path,es_info.ip_addr,es_info.port_,h_buf_len,body_str);

	h_buf_len = frame_strlen(h_buf);
	return h_buf_len;
}

int make_http_get_test(char *h_buf,int len,char *path)
{
	int h_buf_len = 0;
	
	snprintf(h_buf,len,"GET %s HTTP/1.1\r\n\
Host: %s:%d\r\n\
Connection: keep-alive\r\n\
Upgrade-Insecure-Requests: 1\
User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/59.0.3071.86 Safari/537.36\
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\
Accept-Encoding: gzip, deflate\
Accept-Language: zh-CN,zh;q=0.8",path,es_info.ip_addr,es_info.port_);

	h_buf_len = frame_strlen(h_buf);
	return h_buf_len;
}

