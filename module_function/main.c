
#include <time.h>
#include <sys/time.h>

#include "common.h"
#include "frame_tool.h"
#include "log_mini.h"


static struct itimerval itv = {0};

void doing_defore_exiting()
{
	log_flush();
	PERROR("There is prepare exiting and do some clean work\n");
	return;
}

void clock_init()
{
    struct timeval tv = {0};
    struct tm tm_truct = {0};
    int hour = 0;

    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec,&tm_truct);

    PDEBUG("get time of system:: %d-%02d-%02d %02d:%02d:%02d\n",1900+tm_truct.tm_year,1+tm_truct.tm_mon,tm_truct.tm_mday,
		tm_truct.tm_hour,tm_truct.tm_min,tm_truct.tm_sec);

    hour = 	tm_truct.tm_hour;
    if(hour < 6)
    {
        ;
    }

    hour = 24 - hour;

    itv.it_interval.tv_sec= 24 * 3600;
	itv.it_interval.tv_usec= 0;

    itv.it_value.tv_sec = hour*3600;
	itv.it_value.tv_usec= 0;
    
    setitimer(ITIMER_REAL,&itv,NULL);
}

void sig_catch(int sig)
{
	PERROR("!!!!!!!!well, we catch signal in this process :::%d will be exit\n",sig);
	switch(sig)
	{
		case SIGALRM:
			PERROR("SIGALRM :: %d\n",SIGALRM);
			return;
		case SIGINT:
			PERROR("SIGINT :: %d\n",SIGINT);
			break;
		case SIGSEGV:
			PERROR("SIGSEGV :: %d\n",SIGSEGV);
			break;
		case SIGTERM:
			PERROR("SIGTERM :: %d\n",SIGTERM);
			break;
		default:
			PERROR("%d\n",sig);
			break;
	}
	
	doing_defore_exiting();
	exit(0);
	return;
}

int sigignore(int sig)
{
    struct sigaction sa = {0};
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;

    if (sigemptyset(&sa.sa_mask) == -1 || sigaction(sig, &sa, 0) == -1) {
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
	(void)signal(SIGALRM,sig_catch);//timer
	(void)signal(SIGINT, sig_catch);//ctrl + c 
	(void)signal(SIGSEGV, sig_catch);//memory
	(void)signal(SIGTERM, sig_catch);//kill
	if (sigignore(SIGHUP) == -1)
	{
		PERROR("Failed to ignore SIGHUP\n");
    }
	log_set_filename("xiaoxiao",2*1024*1024);
	LOG_ERROR("xiaoxiao error message\n");
	LOG_WARN("xiaoxiao warnning message\n");
	LOG_INFO("xiaoxiao info message\n");
	LOG_DEBUG("xiaoxiao debug message\n");
	LOG_TRACE("xiaoxiao trace message\n");
	doing_defore_exiting();
	return 0;
}

