
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
    int addrlen,n;
    struct sockaddr_in addr_ser;
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

    addrlen=sizeof(addr_ser);

    while(1)
    {
        printf("Input your words: ");
        scanf("%s",sendline);

        n=sendto(sockfd,sendline,200,0,(struct sockaddr *)&addr_ser,addrlen);
        if(n==-1)
        {
            printf("sendto error:%s\n",strerror(errno));
            return -1;
        }

        printf("waiting for server......\n");
        n=recvfrom(sockfd,recvline,200,0,(struct sockaddr *)&addr_ser,&addrlen);
        if(n==-1)
        {
            printf("recvfrom error:%s\n",strerror(errno));
            return -1;
        }
        recvline[n]='\0';
        printf("recv data is:%s\n",recvline);
    }

    return 0;
}
