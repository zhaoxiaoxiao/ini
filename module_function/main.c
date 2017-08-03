
#include <time.h>
#include <sys/time.h>

#include "common.h"
#include "frame_thread_pool.h"
#include "es_http_api.h"

static struct itimerval itv = {0};


void doing_defore_exiting()
{
	es_server_destroy();
	frame_threadpool_destroy();
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
	PERROR("!!!!!!!!well, we catch signal in this process :::%d will be exit\n\n",sig);
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

int main(int argc, char *argv[])
{
	int ret = 0,i = 0;
	THREAD_POOL_JOB job = {0};
	ES_REQUEST req = {0};
	
	(void)signal(SIGALRM,sig_catch);//timer
	(void)signal(SIGINT, sig_catch);//ctrl + c 
	(void)signal(SIGSEGV, sig_catch);//memory
	(void)signal(SIGTERM, sig_catch);//kill

	ret = frame_threadpool_init(4);
	if(ret < 0)
	{
		PERROR("Thread pool init error\n");
		return ret;
	}
	ret = es_server_init("127.0.0.1",50001,HTTP);
	if(ret < 0)
	{
		PERROR("es info init error\n");
		return ret;
	}
	job.call = es_server_send;
	job.arg = NULL;
	frame_add_job_queue(&job);

	job.call = es_server_recv;
	job.arg = NULL;
	frame_add_job_queue(&job);

	job.call = es_asynchronous_callback;
	job.arg = NULL;
	frame_add_job_queue(&job);
#if 1
	for(i = 0;;i ++)
	{
		req.path_str = "/";
		req.query_str = "/_search?pretty";
		req.body_str = NULL;
		req.verb = GET_METHOD;
		ret = es_query_asynchronous(&req,es_asynchronous_test_functon);
		if(ret < 0)
		{
			PERROR("es_query_asynchronous error\n");
			break;
		}

		req.path_str = "/elefence_terminal_info_00";
		req.query_str = "/_search?pretty";
		req.body_str = "{\"query\":{\"match\": { \"terminal_mac\": \"42-57-4A-EA-4D-28\" }}}";
		req.verb = GET_METHOD;
		ret = es_query_asynchronous(&req,es_asynchronous_test_functon);
		if(ret < 0)
		{
			PERROR("es_query_asynchronous error\n");
			break;
		}
	}
#endif
	while(1)
	{
		sleep(60);
	}
	doing_defore_exiting();
	return 0;
}

