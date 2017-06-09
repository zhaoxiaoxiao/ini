
#ifndef __PROGRAM_FRAMEWORK_THREAD_POOL_H__
#define __PROGRAM_FRAMEWORK_THREAD_POOL_H__


#ifdef __cplusplus
extern "C" {
#endif

typedef void (*THREAD_JOB_FUNCTION)(void *data);

typedef struct thread_pool_job{
	THREAD_JOB_FUNCTION call;
	void *arg;
}THREAD_POOL_JOB;

int frame_add_job_queue(THREAD_POOL_JOB *tp_job);

int frame_threadpool_init(int thread_size);

int frame_threadpool_sleep_num();

void frame_threadpool_destroy();

#ifdef __cplusplus
}
#endif
#endif

