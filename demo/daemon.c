
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<stdlib.h>
#include<stdio.h>
#include<syslog.h>
#include<signal.h>

int daemon_init(void)
{
    pid_t pid;

    if((pid=fork())<0)
	return(-1);
    else if(pid!=0)
	exit(0);/*parentexit*/
    /*childcontinues*/
    setsid();/*becomesessionleader*/
    chdir("/");/*changeworkingdirectory*/
    umask(0);/*clearfilemodecreationmask*/
    close(0);/*closestdin*/
    close(1);/*closestdout*/
    close(2);/*closestderr*/
    return(0);
}
void sig_term(int signo)
{
    if(signo==SIGTERM) /*catchedsignalsentbykill(1)command*/
    {
        syslog(LOG_INFO,"programterminated.");
        closelog();
        exit(0);
    }
}

int main(void)
{
    if(daemon_init()==-1)
    {
        printf("can'tforkself\n");
        exit(0);
    }
    openlog("daemon",LOG_PID,LOG_USER);
    syslog(LOG_INFO,"programstarted.");
    signal(SIGTERM,sig_term);/*arrangetocatchthesignal*/
    while(1)
    {
        sleep(10);/*putyourmainprogramhere*/
    }
    return(0);
}
