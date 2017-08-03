
#include "common.h"
#include "frame_thread_pool.h"
#include "es_http_api.h"
#include "es_test_function.h"


void es_test_function()
{
	int ret = 0,i = 0;
	THREAD_POOL_JOB job = {0};
	ES_REQUEST req = {0};
	
	ret = frame_threadpool_init(4);
	if(ret < 0)
	{
		PERROR("Thread pool init error\n");
		return;
	}
	ret = es_server_init("127.0.0.1",50001,HTTP);
	if(ret < 0)
	{
		PERROR("es info init error\n");
		return;
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
}


void es_test_exit()
{
	es_server_destroy();
	frame_threadpool_destroy();
}

