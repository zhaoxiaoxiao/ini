
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/socket.h>  
#include <sys/types.h>  
#include <unistd.h>  
#include <netinet/in.h>  

#define PORT 6666  

int main(int argc,char **argv)  
{
    int sockfd;
    int err,n;
    struct sockaddr_in addr_ser;
    char sendline[20],recvline[20];

    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd==-1)
    {
        printf("socket error\n");
        return -1;
    }

    bzero(&addr_ser,sizeof(addr_ser));
    addr_ser.sin_family=AF_INET;
    addr_ser.sin_addr.s_addr=htonl(INADDR_ANY);
    addr_ser.sin_port=htons(PORT);
    err=connect(sockfd,(struct sockaddr *)&addr_ser,sizeof(addr_ser));
    if(err==-1)
    {
        printf("connect error\n");
        return -1;
    }
      
    printf("connect with server...\n");

    while(1)
    {
        printf("Input your words:");
        scanf("%s",&sendline);
          
        send(sockfd,sendline,strlen(sendline),0);
      
        printf("waiting for server...\n");
      
        n=recv(sockfd,recvline,100,0);
        recvline[n]='\0';
          
        printf("recv data is:%s\n",recvline);
    }

    return 0;  
}