
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <assert.h>

#define IPADDR      "172.16.10.111"
#define PORT        10323
#define MAXLINE     1024
#define LISTENQ     5
#define SIZE        10

typedef struct server_context_st
{
    int cli_cnt;        /*�ͻ��˸���*/
    int clifds[SIZE];   /*�ͻ��˵ĸ���*/
    fd_set allfds;      /*�������*/
    int maxfd;          /*������ֵ*/
} server_context_st;
static server_context_st *s_srv_ctx = NULL;
/*===========================================================================
 * ==========================================================================*/
static int create_server_proc(const char* ip,int port)
{
    int  fd;
    struct sockaddr_in servaddr;
    fd = socket(AF_INET, SOCK_STREAM,0);
    if (fd == -1) {
        fprintf(stderr, "create socket fail,erron:%d,reason:%s\n",
                errno, strerror(errno));
        return -1;
    }

    /*һ���˿��ͷź��ȴ�������֮������ٱ�ʹ�ã�SO_REUSEADDR���ö˿��ͷź������Ϳ��Ա��ٴ�ʹ�á�*/
    int reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        return -1;
    }

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&servaddr.sin_addr);
    servaddr.sin_port = htons(port);

    if (bind(fd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1) {
        perror("bind error: ");
        return -1;
    }

    listen(fd,LISTENQ);

    return fd;
}

static int accept_client_proc(int srvfd)
{
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen;
    cliaddrlen = sizeof(cliaddr);
    int clifd = -1;

    printf("accpet clint proc is called.\n");

ACCEPT:
    clifd = accept(srvfd,(struct sockaddr*)&cliaddr,&cliaddrlen);

    if (clifd == -1) {
        if (errno == EINTR) {
            goto ACCEPT;
        } else {
            fprintf(stderr, "accept fail,error:%s\n", strerror(errno));
            return -1;
        }
    }

    fprintf(stdout, "accept a new client: %s:%d\n",
            inet_ntoa(cliaddr.sin_addr),cliaddr.sin_port);

    //���µ�������������ӵ�������
    int i = 0;
    for (i = 0; i < SIZE; i++) {
        if (s_srv_ctx->clifds[i] < 0) {
            s_srv_ctx->clifds[i] = clifd;
            s_srv_ctx->cli_cnt++;
            break;
        }
    }

    if (i == SIZE) {
        fprintf(stderr,"too many clients.\n");
        return -1;
    }
}

static int handle_client_msg(int fd, char *buf) 
{
    assert(buf);
    printf("recv buf is :%s\n", buf);
    write(fd, buf, strlen(buf) +1);
    return 0;
}

static void recv_client_msg(fd_set *readfds)
{
    int i = 0, n = 0;
    int clifd;
    char buf[MAXLINE] = {0};
    for (i = 0;i <= s_srv_ctx->cli_cnt;i++) {
        clifd = s_srv_ctx->clifds[i];
        if (clifd < 0) {
            continue;
        }
        /*�жϿͻ����׽����Ƿ�������*/
        if (FD_ISSET(clifd, readfds)) {
            //���տͻ��˷��͵���Ϣ
            n = read(clifd, buf, MAXLINE);
            if (n <= 0) {
                /*n==0��ʾ��ȡ��ɣ��ͻ����ر��׽���*/
                FD_CLR(clifd, &s_srv_ctx->allfds);
                close(clifd);
                s_srv_ctx->clifds[i] = -1;
                continue;
            }
            handle_client_msg(clifd, buf);
        }
    }
}
static void handle_client_proc(int srvfd)
{
    int  clifd = -1;
    int  retval = 0;
    fd_set *readfds = &s_srv_ctx->allfds;
    struct timeval tv;
    int i = 0;

    while (1) {
        /*ÿ�ε���selectǰ��Ҫ���������ļ���������ʱ�䣬��Ϊ�¼��������ļ���������ʱ�䶼���ں��޸���*/
        FD_ZERO(readfds);
        /*��Ӽ����׽���*/
        FD_SET(srvfd, readfds);
        s_srv_ctx->maxfd = srvfd;

        tv.tv_sec = 30;
        tv.tv_usec = 0;
        /*��ӿͻ����׽���*/
        for (i = 0; i < s_srv_ctx->cli_cnt; i++) {
            clifd = s_srv_ctx->clifds[i];
            /*ȥ����Ч�Ŀͻ��˾��*/
            if (clifd != -1) {
                FD_SET(clifd, readfds);
            }
            s_srv_ctx->maxfd = (clifd > s_srv_ctx->maxfd ? clifd : s_srv_ctx->maxfd);
        }

        /*��ʼ��ѯ���մ������˺Ϳͻ����׽���*/
        retval = select(s_srv_ctx->maxfd + 1, readfds, NULL, NULL, &tv);
        if (retval == -1) {
            fprintf(stderr, "select error:%s.\n", strerror(errno));
            return;
        }
        if (retval == 0) {
            fprintf(stdout, "select is timeout.\n");
            continue;
        }
        if (FD_ISSET(srvfd, readfds)) {
            /*�����ͻ�������*/
            accept_client_proc(srvfd);
        } else {
            /*���ܴ���ͻ�����Ϣ*/
            recv_client_msg(readfds);
        }
    }
}

static void server_uninit()
{
    if (s_srv_ctx) {
        free(s_srv_ctx);
        s_srv_ctx = NULL;
    }
}

static int server_init()
{
    s_srv_ctx = (server_context_st *)malloc(sizeof(server_context_st));
    if (s_srv_ctx == NULL) {
        return -1;
    }

    memset(s_srv_ctx, 0, sizeof(server_context_st));

    int i = 0;
    for (;i < SIZE; i++) {
        s_srv_ctx->clifds[i] = -1;
    }

    return 0;
}

int main(int argc,char *argv[])
{
    int  srvfd;
    /*��ʼ�������context*/
    if (server_init() < 0) {
        return -1;
    }
    /*��������,��ʼ�����ͻ�������*/
    srvfd = create_server_proc(IPADDR, PORT);
    if (srvfd < 0) {
        fprintf(stderr, "socket create or bind fail.\n");
        goto err;
    }
    /*��ʼ���ղ�����ͻ�������*/
    handle_client_proc(srvfd);
    server_uninit();
    return 0;
err:
    server_uninit();
    return -1;
}

