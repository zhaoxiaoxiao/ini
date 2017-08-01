
#include <pthread.h>
#include <semaphore.h>

#include "common.h"
#include "frame_thread_pool.h"


typedef struct frame_thread_pool{
	int max_thread_num;
	pthread_t *thread_id;
	pthread_mutex_t pool_lock;
	int sleep_thread_num;
	int is_quit;

	sem_t job_free_sem;
	sem_t job_valid_sem;
	pthread_mutex_t job_free_lock;
	pthread_mutex_t job_valid_lock;
	int push_index;
	int pop_index;
	int job_queue_size;
	THREAD_POOL_JOB *q_head;
}FRAME_THREAD_POOL;

static FRAME_THREAD_POOL thread_pool_ = {0};
///////////////////////////////////////////////////
int get_free_job_index()
{
	int index = 0;
	sem_wait(&thread_pool_.job_free_sem);
	pthread_mutex_lock(&thread_pool_.job_free_lock);
	index =  thread_pool_.push_index;
	thread_pool_.push_index++;
	if(thread_pool_.push_index == thread_pool_.job_queue_size)
		thread_pool_.push_index = 0;
	pthread_mutex_unlock(&thread_pool_.job_free_lock);
	return index;
}

void post_job_free_num()
{
	sem_post(&thread_pool_.job_free_sem);
}

void post_job_valid_num()
{
	sem_post(&thread_pool_.job_valid_sem);
}

int get_valid_job_index()
{
	int index = 0;
	sem_wait(&thread_pool_.job_valid_sem);
	pthread_mutex_lock(&thread_pool_.job_valid_lock);
	index = thread_pool_.pop_index;
	thread_pool_.pop_index++;
	if(thread_pool_.pop_index == thread_pool_.job_queue_size)
		thread_pool_.pop_index = 0;
	pthread_mutex_unlock(&thread_pool_.job_valid_lock);
	return index;
}

void frame_threadpool_sleep()
{
	pthread_mutex_lock(&thread_pool_.pool_lock);
	thread_pool_.sleep_thread_num++;
	pthread_mutex_unlock(&thread_pool_.pool_lock);

	//__sync_fetch_and_add(&thread_pool_.sleep_thread_num,1);
}

void frame_threadpool_wakeup()
{
	pthread_mutex_lock(&thread_pool_.pool_lock);
	thread_pool_.sleep_thread_num--;
	pthread_mutex_unlock(&thread_pool_.pool_lock);
}

void* thread_routine(void *arg)
{
	THREAD_POOL_JOB *curr_job = NULL;
	int index = 0;
	while(1)
	{
		PDEBUG("Thread routing start....\n");
		frame_threadpool_sleep();
		index = get_valid_job_index();
		frame_threadpool_wakeup();
		
		curr_job = &thread_pool_.q_head[index];
		if(curr_job->call)
			curr_job->call(curr_job->arg);

		curr_job->call = NULL;
		curr_job->arg = NULL;
		post_job_free_num();
		if(thread_pool_.is_quit)
			break;
	}
	
	pthread_exit(NULL);
}

///////////////////////////////////////////////////
int frame_threadpool_init(int thread_size)
{
	int i = 0,ret = 0;
	pthread_attr_t attr;
	
	memset(&thread_pool_,0,sizeof(FRAME_THREAD_POOL));
	thread_pool_.max_thread_num = thread_size;
	
	thread_pool_.thread_id = (pthread_t *) malloc (thread_size * sizeof (pthread_t));
	if(thread_pool_.thread_id == NULL)
	{
		goto error_out;
	}

	thread_pool_.q_head = (THREAD_POOL_JOB *) malloc (thread_size * sizeof (THREAD_POOL_JOB));
	if(thread_pool_.q_head == NULL)
	{
		goto error_out;
	}
	thread_pool_.job_queue_size = thread_size;
	thread_pool_.push_index = 0;
	thread_pool_.pop_index = 0;

	sem_init(&thread_pool_.job_valid_sem, 0, 0);
	sem_init(&thread_pool_.job_free_sem, 0, thread_size);

	pthread_mutex_init(&thread_pool_.pool_lock,NULL);
	pthread_mutex_init(&thread_pool_.job_free_lock,NULL);
	pthread_mutex_init(&thread_pool_.job_valid_lock,NULL);

	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED ); 
	for(i = 0;i < thread_size;i++)
	{
		ret = pthread_create(&thread_pool_.thread_id[i], &attr, thread_routine,NULL);
		if(ret)
		{
			goto error_out;
		}
		pthread_detach(thread_pool_.thread_id[i]);
	}
	
	pthread_attr_destroy(&attr);
	return 0;
error_out:
	if(thread_pool_.thread_id)
		free(thread_pool_.thread_id);
	if(thread_pool_.q_head)
		free(thread_pool_.q_head);
	return -1;
}

int frame_add_job_queue(THREAD_POOL_JOB *tp_job)
{
	int second = 0,index = 0,before = 0,after = 0;
	THREAD_POOL_JOB *curr_job = NULL;

	if(thread_pool_.is_quit)
		return -1;
	
	before;
	index = get_free_job_index();
	after;
	second =after - before;

	curr_job = &thread_pool_.q_head[index];
	curr_job->arg = tp_job->arg;
	curr_job->call = tp_job->call;
	post_job_valid_num();
}

int frame_threadpool_sleep_num()
{
	return thread_pool_.sleep_thread_num;
}

void frame_threadpool_destroy()
{
	int i = 0;
	thread_pool_.is_quit = 1;
	for(i = 0;i < thread_pool_.job_queue_size;i++)
	{
		post_job_valid_num();
	}

	if(thread_pool_.thread_id)
		free(thread_pool_.thread_id);
	if(thread_pool_.q_head)
		free(thread_pool_.q_head);
}

