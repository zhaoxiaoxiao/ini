
#include <fcntl.h>
#include <netdb.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "common.h"
#include "socket_api.h"

void setNonBlockAndCloseOnExec(int sockfd)
{
	int ret = 0,flags = 0;
	// non-block
	flags = ::fcntl(sockfd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	ret = fcntl(sockfd, F_SETFL, flags);
	// FIXME check

	// close-on-exec
	flags = ::fcntl(sockfd, F_GETFD, 0);
	flags |= FD_CLOEXEC;
	ret = fcntl(sockfd, F_SETFD, flags);
	// FIXME check

	(void)ret;
}

void connect_time_out(int fd,int out_sec,int out_us)
{
	struct timeval timeout;
	int len = sizeof(struct timeval),ret = 0;
	
    timeout.tv_sec = out_sec;    
    timeout.tv_usec = out_us;
	ret = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
    if (ret < 0)
    {
    	PERROR("The return num ::%d\n",ret);
    }
	return;
}

void recv_time_out(int fd,int second)
{
	struct timeval timeout;
	int ret = 0,len = sizeof(struct timeval);
	timeout.tv_sec = second;    
    timeout.tv_usec = 0;
    ret = setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&(timeout.tv_sec),len);
    if (ret < 0)
    {
    	PERROR("The return num ::%d\n",ret);
    }
    return;
}

void setTcpNoDelay(int fd,int onff)
{
	int ret = 0;
	if(onff)
		onff = 1;
	else
		onff = 0;
	ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &onff, static_cast<socklen_t>(sizeof onff));
	if(ret != 0)
	{
		PERROR("The return num ::%d\n",ret);
	}
}

void setReuseAddr(int fd,int onff)
{
	int ret = 0;
	if(onff)
		onff = 1;
	else
		onff = 0;
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &onff, static_cast<socklen_t>(sizeof onff));
	if(ret != 0)
	{
		PERROR("The return num ::%d\n",ret);
	}
}

void setReusePort(int fd,int onff)
{
	int ret = 0;
	if(onff)
		onff = 1;
	else
		onff = 0;
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &onff, static_cast<socklen_t>(sizeof onff));
	if(ret != 0)
	{
		PERROR("The return num ::%d\n",ret);
	}
}

void setKeepAlive(int fd,int onff)
{
	int ret = 0;
	if(onff)
		onff = 1;
	else
		onff = 0;
	ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &onff, static_cast<socklen_t>(sizeof onff));
	if(ret != 0)
	{
		PERROR("The return num ::%d\n",ret);
	}
}

int socket_tcp_ipv4()
{
	int s_fd = 0;
	s_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(s_fd < 0)
	{
		PERROR("The return error num :: %d\n",s_fd);
	}
	return s_fd;
}

int socket_tcp_ipv6()
{
	int s_fd = 0;
	s_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	if(s_fd < 0)
	{
		PERROR("The return error num :: %d\n",s_fd);
	}
	return s_fd;
}

int socket_udp_ipv4()
{
	int s_fd = 0;
	s_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(s_fd < 0)
	{
		PERROR("The return error num :: %d\n",s_fd);
	}
	return s_fd;
}

int socket_udp_ipv6()
{
	int s_fd = 0;
	s_fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if(s_fd < 0)
	{
		PERROR("The return error num :: %d\n",s_fd);
	}
	return s_fd;
}

int bind_tcp_server(int s_fd,unsigned short port,const char *ip_str)
{
	int ret = 0;
	socklen_t addr_len = 0;
	struct sockaddr_in server_addr = {0};

	addr_len = sizeof(struct sockaddr_in);

	server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(ip_str);

	ret = bind(s_fd, (struct sockaddr*)&server_addr, addr_len);

	if(ret < 0)
	{
		PERROR("The return num ::%d\n",ret);
	}

	return ret;
}

int connect_tcp_server(int s_fd,unsigned short port,const char *ip_str)
{
	int ret = 0;
	socklen_t addr_len = 0;
	struct sockaddr_in server_addr = {0};

	addr_len = sizeof(struct sockaddr_in);

	server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(ip_str);

	ret = connect(s_fd, (struct sockaddr*)&server_addr, addr_len);

	if(ret < 0)
	{
		PERROR("The return num ::%d\n",ret);
	}

	return ret;
}

int analy_ipstr_dns(const char *domain,char *ip_str,int str_len)
{
	const char *p = NULL;
	char **pp = NULL;
	struct hostent *host = NULL;
	/**
	struct hostent
    {
        char    *h_name;               
        char    **h_aliases;
        int     h_addrtype;
        int     h_length;
        char    **h_addr_list;
        #define h_addr h_addr_list[0]
    };
	**/
	host = gethostbyname(domain);
	if(host == NULL)
		return -1;

	if(host->h_addrtype == AF_INET || host->h_addrtype == AF_INET6)
	{
		pp = host->h_addr_list;
		if(*pp)
		{
			p = inet_ntop(host->h_addrtype, *pp, ip_str, str_len);
		}
	}

	if(p == NULL)
		return -1;
	PERROR("%s\n",p);
	return 0;
}

int tcp_client_recv(int fd,char *r_buf,int len)
{
	char *wp = r_buf;
	int recv_len = 0;
recv_again:	
	memset(wp,0,len);
	recv_len = recv(fd, wp, len, 0);
	if(recv_len < 0)
	{
		PERROR("The return num :: %d and error :: %d\n",recv_len,errno);
		switch(errno)
		{
			case EAGAIN:
				PERROR("EAGAIN :: %d\n",errno);
				recv_len = len + 1;
				break;
			case EBADF:
				PERROR("EBADF :: %d\n",errno);
				break;
			case EFAULT:
				PERROR("EFAULT :: %d\n",errno);
				break;
			case EINTR:
				PERROR("EINTR :: %d\n",errno);
				goto recv_again;;
			case EINVAL:
				PERROR("EINVAL :: %d\n",errno);
				break;
			case ENOMEM:
				PERROR("ENOMEM :: %d\n",errno);
				break;
			case ENOTCONN:
				PERROR("ENOTCONN :: %d\n",errno);
				break;
			case ENOTSOCK:
				PERROR("ENOTSOCK :: %d\n",errno);
				break;
			default:
				PERROR("%d\n",errno);
				break;
		}
	}

	return recv_len;
}

int tcp_client_send(int fd,const char *s_buf)
{
	const char *cp = s_buf;
	unsigned int len = 0,s_len = 0, n_len = strlen(s_buf);
	while(len < n_len)
	{
		cp += s_len;
		s_len = n_len - len;
send_again:		
		s_len = send(fd, cp, s_len, 0);
		if(s_len < 0)
		{
			PERROR("The return num :: %d and error :: %d\n",s_len,errno);
			switch(errno)
			{
				case EAGAIN:
					PERROR("EAGAIN :: %d\n",errno);
					break;
				case EBADF:
					PERROR("EBADF :: %d\n",errno);
					break;
				case EFAULT:
					PERROR("EFAULT :: %d\n",errno);
					break;
				case EINTR:
					PERROR("EINTR :: %d\n",errno);
					goto send_again;;
				case EINVAL:
					PERROR("EINVAL :: %d\n",errno);
					break;
				case ENOMEM:
					PERROR("ENOMEM :: %d\n",errno);
					break;
				case ENOTCONN:
					PERROR("ENOTCONN :: %d\n",errno);
					break;
				case ENOTSOCK:
					PERROR("ENOTSOCK :: %d\n",errno);
					break;
				default:
					PERROR("%d\n",errno);
					break;
			}
		}else{
			len += s_len;
		}
	}
	return len;
}

////////////////////////////////////////////////////////////////////////////////////////////////
int getTcpInfo(int s_fd,struct tcp_info* tcpi)
{
	int ret = 0;
	socklen_t len = sizeof(*tcpi);
	bzero(tcpi, len);
	ret = getsockopt(s_fd, SOL_TCP, TCP_INFO, tcpi, &len);
	if(ret != 0)
	{
		PERROR("The return num ::%d\n",ret);
	}
	return ret;
}

int getTcpInfoString(int s_fd,char* buf, int len)
{
	int ret = 0;
	struct tcp_info tcpi;
	ret = getTcpInfo(s_fd,&tcpi);
	if (ret == 0)
	{
		snprintf(buf, len, "unrecovered=%u "
			 "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
			 "lost=%u retrans=%u rtt=%u rttvar=%u "
			 "sshthresh=%u cwnd=%u total_retrans=%u",
			 tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
			 tcpi.tcpi_rto, 		 // Retransmit timeout in usec
			 tcpi.tcpi_ato, 		 // Predicted tick of soft clock in usec
			 tcpi.tcpi_snd_mss,
			 tcpi.tcpi_rcv_mss,
			 tcpi.tcpi_lost,		 // Lost packets
			 tcpi.tcpi_retrans, 	 // Retransmitted packets out
			 tcpi.tcpi_rtt, 		 // Smoothed round trip time in usec
			 tcpi.tcpi_rttvar,		 // Medium deviation
			 tcpi.tcpi_snd_ssthresh,
			 tcpi.tcpi_snd_cwnd,
			 tcpi.tcpi_total_retrans);	// Total retransmits for entire connection
	}
	return ret;
}


