
#include <stdio.h> 
#include <unistd.h>////close
#include <string.h> 
#include <stdlib.h> 
#include <sys/socket.h>
#include <errno.h> 
#include <netdb.h> 
#include <arpa/inet.h>

#include "frame_tool.h"
#include "tcp_ack_scan.h"

struct tcphdr
{
    u_int16_t source;
    u_int16_t dest;
    u_int32_t seq;
    u_int32_t ack_seq;
#if __BYTE_ORDER == __LITTLE_ENDIAN
    u_int16_t res1:4;
    u_int16_t doff:4;
    u_int16_t fin:1;
    u_int16_t syn:1;
    u_int16_t rst:1;
    u_int16_t psh:1;
    u_int16_t ack:1;
    u_int16_t urg:1;
    u_int16_t res2:2;
#elif __BYTE_ORDER == __BIG_ENDIAN
    u_int16_t doff:4;
    u_int16_t res1:4;
    u_int16_t res2:2;
    u_int16_t urg:1;
    u_int16_t ack:1;
    u_int16_t psh:1;
    u_int16_t rst:1;
    u_int16_t syn:1;
    u_int16_t fin:1;
#  else
#   error "Adjust your <bits/endian.h> defines"
#  endif
    u_int16_t window;
    u_int16_t check;
    u_int16_t urg_ptr;
};

struct ip
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ip_hl:4;		/* header length */
    unsigned int ip_v:4;		/* version */
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
    unsigned int ip_v:4;		/* version */
    unsigned int ip_hl:4;		/* header length */
#endif
    u_int8_t ip_tos;			/* type of service */
    u_short ip_len;				/* total length */
    u_short ip_id;				/* identification */
    u_short ip_off;				/* fragment offset field */
#define	IP_RF 0x8000			/* reserved fragment flag */
#define	IP_DF 0x4000			/* dont fragment flag */
#define	IP_MF 0x2000			/* more fragments flag */
#define	IP_OFFMASK 0x1fff		/* mask for fragmenting bits */
    u_int8_t ip_ttl;			/* time to live */
    u_int8_t ip_p;			/* protocol */
    u_short ip_sum;			/* checksum */
    struct in_addr ip_src, ip_dst;	/* source and dest address */
};

struct iphdr 
{ 
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ihl:4;
    unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int version:4;
    unsigned int ihl:4;
#else
# error  "Please fix <bits/endian.h>"
#endif
    u_int8_t tos;
    u_int16_t tot_len;
    u_int16_t id;
    u_int16_t frag_off;
    u_int8_t ttl;
    u_int8_t protocol;
    u_int16_t check;
    u_int32_t saddr;
    u_int32_t daddr;    
};

struct csum_header    //needed for checksum calculation
{
    unsigned int source_address;
    unsigned int dest_address;
    unsigned char placeholder;
    unsigned char protocol;
    unsigned short tcp_length;
    struct tcphdr tcp;
};

#if 1
#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: DEBUG " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)

////////////////////////////////////////////////////////////////////////////////////////////////////
static struct in_addr dest_ip = {0};

int get_source_ip ( char* buffer)
{
    int sock = socket ( AF_INET, SOCK_DGRAM, 0);
 
    const char* kGoogleDnsIp = "8.8.8.8";
    int dns_port = 53;
 
    struct sockaddr_in serv;
 
    memset( &serv, 0, sizeof(serv) );
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
    serv.sin_port = htons( dns_port );
 
    int err = connect( sock , (const struct sockaddr*) &serv , sizeof(serv) );
 
    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    err = getsockname(sock, (struct sockaddr*) &name, &namelen);
 
    const char *p = inet_ntop(AF_INET, &name.sin_addr, buffer, 100);
 
    close(sock);
}

unsigned short checksum(unsigned short *ptr,int nbytes) 
{
    register long sum;
    unsigned short oddbyte;
    register short answer;
 
    sum=0;
    while(nbytes>1) {
        sum+=*ptr++;
        nbytes-=2;
    }
    if(nbytes==1) {
        oddbyte=0;
        *((u_char*)&oddbyte)=*(u_char*)ptr;
        sum+=oddbyte;
    }
 
    sum = (sum>>16)+(sum & 0xffff);
    sum = sum + (sum>>16);
    answer=(short)~sum;
     
    return(answer);
}


int check_rst_flag(unsigned char* buffer, int size)
{
    struct iphdr *iph = (struct iphdr*)buffer;
    struct sockaddr_in source,dest;
    unsigned short iphdrlen;
     
    if(iph->protocol == 6)
    {
        struct iphdr *iph = (struct iphdr *)buffer;
        iphdrlen = iph->ihl*4;
     
        struct tcphdr *tcph=(struct tcphdr*)(buffer + iphdrlen);
             
        memset(&source, 0, sizeof(source));
        source.sin_addr.s_addr = iph->saddr;
     
        memset(&dest, 0, sizeof(dest));
        dest.sin_addr.s_addr = iph->daddr;
		//printf("\n rst %d : ", tcph->rst);         
        if(tcph->rst == 1 && source.sin_addr.s_addr == dest_ip.s_addr)
        {
            fflush(stdout);
			return 1;
		}else
		{
			fflush(stdout);
            return 0;
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int tcp_ack_scan_server_port(const char *ip_addr)
{
	struct iphdr *iph = NULL;
	struct tcphdr *tcph = NULL;
	struct sockaddr_in  dest = {0};
    struct csum_header psh = {0};
	struct sockaddr saddr = {0};
	char datagram[4096],source_ip[20];
	unsigned char *buffer = NULL;
	socklen_t saddr_size = 0;
	int i = 0,s_fd = 0,source_port = 43591,one = 1,sock_id = 0,data_size = 0;
	const int *val = &one;

	s_fd = socket (AF_INET, SOCK_RAW , IPPROTO_TCP);
	if(s_fd < 0)
	{
		PERROR("system socket error:: %d %s\n",errno , strerror(errno));
		return -3;
	}

	iph = (struct iphdr *) datagram;

	tcph = (struct tcphdr *) (datagram + sizeof (struct ip));

	if(inet_addr(ip_addr) != -1)
	{
		dest_ip.s_addr = inet_addr(ip_addr);
	}

	get_source_ip(source_ip);
	PDEBUG("Local Machine IP is %s\n", source_ip);
	memset (datagram, 0, 4096);

	//Fill in the IP Header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof (struct ip) + sizeof (struct tcphdr);
    iph->id = htons (54321); //Id of this packet
    iph->frag_off = htons(16384);
    iph->ttl = 64;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0;      //Set to 0 before calculating checksum
    iph->saddr = inet_addr ( source_ip );    
    iph->daddr = dest_ip.s_addr;
	iph->check = checksum ((unsigned short *) datagram, iph->tot_len >> 1);

	//TCP Header
    tcph->source = htons ( source_port );
    tcph->dest = htons (8099);
    tcph->seq = htonl(1105024978);
    tcph->ack_seq = 0;
    tcph->doff = sizeof(struct tcphdr) / 4;      //Size of tcp header
    tcph->fin=0;
    tcph->syn=0;
    tcph->rst=0;
    tcph->psh=0;
    tcph->ack=1;
    tcph->urg=0;
    tcph->window = htons ( 14600 );  
    tcph->check = 0; 
    tcph->urg_ptr = 0;

	//IP_HDRINCL to tell the kernel that headers are included in the packet
    if (setsockopt (s_fd, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)
    {
        PERROR("Error setting IP_HDRINCL. Error number : %d . Error message : %s \n" , errno , strerror(errno));
        return -3;
    }

    sock_id = socket(AF_INET , SOCK_RAW , IPPROTO_TCP);
    if(sock_id < 0)
    {
        PERROR("Socket Error\n");
        fflush(stdout);
        return -3;
    }

	buffer = (unsigned char *)malloc(65536); //Its Big!
    PDEBUG("Starting to send ack packets\n");
    for(i = 0;i < 1024;i++)
    {
    	dest.sin_family = AF_INET;
    	dest.sin_addr.s_addr = dest_ip.s_addr;
        tcph->dest = htons (i);
        tcph->check = 0;

		psh.source_address = inet_addr( source_ip );
        psh.dest_address = dest.sin_addr.s_addr;
        psh.placeholder = 0;
        psh.protocol = IPPROTO_TCP;
        psh.tcp_length = htons( sizeof(struct tcphdr) );

		memcpy(&psh.tcp , tcph , sizeof (struct tcphdr));

		tcph->check = checksum( (unsigned short*) &psh , sizeof (struct csum_header));

		//Send the packet
        if ( sendto (s_fd, datagram , sizeof(struct iphdr) + sizeof(struct tcphdr) , 0 , (struct sockaddr *) &dest, sizeof (dest)) < 0)
        {
            PERROR("Error sending syn packet. Error number : %d . Error message : %s \n" , errno , strerror(errno));
            return -3;
        }
		fflush(stdout);
     	saddr_size = sizeof saddr;

		//Receive a packet
		data_size = recvfrom(sock_id, buffer , 65536 , 0 , &saddr , &saddr_size);
		//PDEBUG("data size:%d\n",data_size);
        if(data_size <0 )
        {
            PERROR("Recvfrom error , failed to get packets\n");
            fflush(stdout);
            return -3;
        }

		if(check_rst_flag(buffer,data_size) > 0)
		{
			PERROR("port :: %d open\n",i);
		}
    }

	close(sock_id);
 	fflush(stdout);
 	return 0;
}


