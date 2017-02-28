
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/socket.h>  
#include <sys/types.h>  
#include <unistd.h>  
#include <netinet/in.h>  
#include <errno.h>    

#define PORT 6666  

int main(int argc,char **argv)  
{  
    int sockfd;
    int err,n;
    int addrlen;
    struct sockaddr_in addr_ser,addr_cli;
    char recvline[200],sendline[200];

    sockfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd==-1)
    {
        printf("socket error:%s\n",strerror(errno));
        return -1;
    }

    bzero(&addr_ser,sizeof(addr_ser));
    addr_ser.sin_family=AF_INET;
    addr_ser.sin_addr.s_addr=htonl(INADDR_ANY);
    addr_ser.sin_port=htons(PORT);
    err=bind(sockfd,(struct sockaddr *)&addr_ser,sizeof(addr_ser));
    if(err==-1)
    {
        printf("bind error:%s\n",strerror(errno));
        return -1;
    }

    addrlen=sizeof(struct sockaddr);
 
    while(1)
    {
        printf("waiting for client......\n");
        n=recvfrom(sockfd,recvline,200,0,(struct sockaddr *)&addr_cli,&addrlen);
        if(n==-1)
        {
            printf("recvfrom error:%s\n",strerror(errno));
            return -1;
        }
        recvline[n]='\0';
        printf("recv data is:%s\n",recvline);

        printf("Input your words: \n");
        scanf("%s",sendline);

        n=sendto(sockfd,sendline,200,0,(struct sockaddr *)&addr_cli,addrlen);
        if(n==-1)
        {
            printf("sendto error:%s\n",strerror(errno));
            return -1;
        }
    }

    return 0;
}