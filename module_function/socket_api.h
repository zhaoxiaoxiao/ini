
#ifndef __PROGRAM_FRAMEWORK_SOCKETAPI_H__
#define __PROGRAM_FRAMEWORK_SOCKETAPI_H__

#ifdef __cplusplus
extern "C" {
#endif

void setNonBlockAndCloseOnExec(int sockfd);

void connect_time_out(int fd,int out_sec,int out_us);

void recv_time_out(int fd,int second);

void setTcpNoDelay(int fd,int onff);

void setReuseAddr(int fd,int onff);

void setReusePort(int fd,int onff);

void setKeepAlive(int fd,int onff);

int socket_tcp_ipv4();

int socket_tcp_ipv6();

int socket_udp_ipv4();

int socket_udp_ipv6();

int bind_tcp_server(int s_fd,unsigned short port,const char *ip_str);

int connect_tcp_server(int s_fd,unsigned short port,const char *ip_str);

int analy_ipstr_dns(const char *domain,char *ip_str,int str_len);

int tcp_client_recv(int fd,char *r_buf,int len);

int tcp_client_send(int fd,const char *s_buf);

#ifdef __cplusplus
}
#endif

#endif

