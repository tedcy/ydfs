#ifndef AIO_H
#define AIO_H

/*
**moved now
push(*a)
{
	LOCK
	tail->next = a;
	tail = a;
	UNLOCK
}

pop()
{
	a = head->next;
	if(a == NULL)
		return NULL;
	if(a == tail)
	{
		LOCK
		tail = head;
		UNLOCK
	}
	head->next = a->next;
	return a;
}
*/
#include <pthread.h>

#include "log.h"
#include "share_fun.h"
#include "storage_nio.h"

#define AIO_NONE 0
#define AIO_READ 1
#define AIO_WRIT 2

struct NIO_NODE_T;

typedef struct AIO_WORK_NODE_T aio_work_node_t;
struct AIO_WORK_QUEUE_T;
typedef void aio_proc(aio_work_node_t *work_node);
struct AIO_WORK_NODE_T
{
	char buf[BUF_MAXSIZE];
	int size;
	aio_proc *proc;
	void *arg;
	void *mem_node;
	int fd;
	char status;
	//
	char *file_name;
	//
	aio_work_node_t *next;
	struct AIO_WORK_QUEUE_T *res_queue;
};

typedef struct AIO_WORK_QUEUE_T
{
	aio_work_node_t *head,*tail;
	pthread_mutex_t lock;
	int pipe_fd[2];
	char dummy;
}aio_work_queue_t;

typedef struct AIO_WORK_T
{
	aio_work_queue_t *req_queue;
	int stop;
	char dummy;
	pthread_cond_t req_cond;
	pthread_mutex_t wait_lock;
	pthread_t p_tid_aio;
}aio_work_t;

int aio_create_work_queue(aio_work_t *work);
int aio_create_res_queue(struct NIO_NODE_T *nio_node);
int aio_push_req(aio_work_t *work,aio_work_node_t *work_node,int mask);
int aio_push_res(aio_work_node_t *work_node);
aio_work_node_t* aio_pop_req(aio_work_queue_t *req_queue);
aio_work_node_t* aio_pop_res(aio_work_queue_t *res_queue);
void aio_process_res_queue(aio_work_queue_t *aio_work_queue,void *eventLoop);
void *aio_process_req_queue(void *arg);
int aio_main(aio_work_t *work);
int aio_delete_work_queue(aio_work_t *work);

#endif
