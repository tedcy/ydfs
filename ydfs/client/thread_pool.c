#include "thread_pool.h"

CThread_pool *pool = NULL;

static void *thread_routine(void *arg);

void thread_init(int max_thread_num)
{
	pool = (CThread_pool*)malloc(sizeof(CThread_pool));
	
	pthread_mutex_init(&(pool->queue_lock),NULL);
	pthread_cond_init(&(pool->queue_ready),NULL);

	pool->queue_head = NULL;
	pool->max_thread_num = max_thread_num;
	pool->shutdown = 0;
	
	pool->threadid = (pthread_t*)malloc(max_thread_num*sizeof(pthread_t));
	int i;
	for(i = 0;i != max_thread_num;++i)
		pthread_create(&pool->threadid[i],NULL,thread_routine,NULL);

}

void pool_add_worker(void *(*process)(void *),void *arg)
{
	CThread_worker *new_worker = (CThread_worker*)malloc(sizeof(CThread_worker));
	new_worker->arg = arg;
	new_worker->process = process;
	new_worker->next = NULL;

	pthread_mutex_lock(&pool->queue_lock);
	
	CThread_worker *temp_worker = pool->queue_head;
	if(temp_worker != NULL)
	{
		while(temp_worker->next != NULL)
			temp_worker = temp_worker->next;
		temp_worker->next = new_worker;
	}else
		pool->queue_head = new_worker;
	
	pool->cur_queue_size++;
	
	pthread_mutex_unlock(&pool->queue_lock);
	
	pthread_cond_signal(&pool->queue_ready);
}

int pool_destroy()
{	
	if(pool->shutdown == 1)
	return -1;
	
	pool->shutdown = 1;

	pthread_cond_broadcast(&pool->queue_ready);
	
	int i;
	for(i = 0;i != pool->max_thread_num;++i)
		pthread_join(pool->threadid[i],NULL);
	free(pool->threadid);
	pool->threadid = NULL;
	
	CThread_worker *head = NULL;
	while(pool->queue_head != NULL)
	{
		head = pool->queue_head;
		pool->queue_head = pool->queue_head->next;
		free(head);
	}
	
	pthread_mutex_destroy(&pool->queue_lock);
	pthread_cond_destroy(&pool->queue_ready);

	free(pool);

	pool = NULL;
	return 0;
}

static void *thread_routine(void *arg)
{
	//printf("starting thread0x%x\n",(unsigned)pthread_self());
	
	while(1)
	{
		pthread_mutex_lock(&pool->queue_lock);
		while(pool->cur_queue_size == 0&&!pool->shutdown)
		{
			//printf ("thread 0x%x is waiting\n", (unsigned)pthread_self());
			pthread_cond_wait(&pool->queue_ready,&pool->queue_lock);
		}
		if(pool->shutdown)
		{
			pthread_mutex_unlock(&pool->queue_lock);
			//printf("thread0x%x will exit\n",(unsigned)pthread_self());
			pthread_exit(NULL);
		}
		
		//printf("thread 0x%xis starting to work\n",(unsigned)pthread_self());

		pool->cur_queue_size--;
		CThread_worker *worker = pool->queue_head;
		pool->queue_head = worker->next;

		pthread_mutex_unlock(&pool->queue_lock);

		(*worker->process)(worker->arg);

		free(worker);
		worker = NULL;
	}
}
