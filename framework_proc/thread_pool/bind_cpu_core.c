
#include <sys/sysinfo.h>

#include "common.h"
#include "bind_cpu_core.h"

void *thread_setaffinity(void *arg)
{
    cpu_set_t mask;
    cpu_set_t get;
    char buf[256];
    int i;
    int j;
    int num = sysconf(_SC_NPROCESSORS_CONF);
    printf("system has %d processor(s)\n", num);

    for (i = 0; i < num; i++) {
        CPU_ZERO(&mask);
        CPU_SET(i, &mask);
        if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0) {
            fprintf(stderr, "set thread affinity failed\n");
        }
        CPU_ZERO(&get);
        if (pthread_getaffinity_np(pthread_self(), sizeof(get), &get) < 0) {
            fprintf(stderr, "get thread affinity failed\n");
        }
        for (j = 0; j < num; j++) {
            if (CPU_ISSET(j, &get)) {
                printf("thread %d is running in processor %d\n", (int)pthread_self(), j);
            }
        }
        j = 0;
        while (j++ < 100000000) {
            memset(buf, 0, sizeof(buf));
        }
    }
    pthread_exit(NULL);
}

void process_setaffinity(void *arg)
{
	cpu_set_t mask;  //CPU�˵ļ���  
	cpu_set_t get;   //��ȡ�ڼ����е�CPU  
	int *a = (int *)arg;   
	printf("the a is:%d\n",*a);  //��ʾ�ǵڼ����߳�  
	int num = sysconf(_SC_NPROCESSORS_CONF);
	CPU_ZERO(&mask);    //�ÿ�  
	CPU_SET(*a,&mask);   //�����׺���ֵ  
	if (sched_setaffinity(0, sizeof(mask), &mask) == -1)//�����߳�CPU�׺���  
	{  
		printf("warning: could not set CPU affinity, continuing...\n");  
	}  
	while (1)  
	{  
		CPU_ZERO(&get);  
		if (sched_getaffinity(0, sizeof(get), &get) == -1)//��ȡ�߳�CPU�׺���  
		{  
			printf("warning: cound not get thread affinity, continuing...\n");  
		}  
		int i;  
		for (i = 0; i < num; i++)  
		{  
			if (CPU_ISSET(i, &get))//�ж��߳����ĸ�CPU���׺���  
			{  
				printf("this thread %d is running processor : %d\n", i,i);  
			}  
		}  
	}  

	return NULL;
}

