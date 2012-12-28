#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

typedef struct worker
{
	void *(*process)(void *arg);
	void *arg;
	struct worker *next;
}CThread_worker;

typedef struct
{
	pthread_mutex_t queue_lock;
	pthread_cond_t queue_ready;

	CThread_worker *queue_head;

	int shutdown;
	pthread_t *threadid;
	int max_thread_num;
	int cur_queue_size;
}CThread_pool;

extern CThread_pool *pool;

void thread_init(int max_thread_num);
void pool_add_worker(void *(*process)(void *),void *arg);
int pool_destroy();
#endif
