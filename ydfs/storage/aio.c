#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "aio.h"

int aio_create_work_queue(aio_work_t *work)
{
	aio_work_queue_t *req_queue;

	work->req_queue = malloc(sizeof(aio_work_queue_t));
	
	req_queue = work->req_queue;

	req_queue->head = malloc(sizeof(aio_work_node_t));
	req_queue->head->next = NULL;
	req_queue->tail = req_queue->head;
	
	pthread_mutex_init(&work->wait_lock,NULL);
	pthread_mutex_init(&work->wait_lock,NULL);
	pthread_mutex_init(&req_queue->lock,NULL);
	return 0;
}

int aio_create_res_queue(struct NIO_NODE_T *nio_node)
{
	nio_node->res_queue = malloc(sizeof(aio_work_queue_t));	
	nio_node->res_queue->head = malloc(sizeof(aio_work_node_t));
	nio_node->res_queue->head->next = NULL;
	nio_node->res_queue->tail = nio_node->res_queue->head;

	pthread_mutex_init(&nio_node->res_queue->lock,NULL);

	return 0;
}

int aio_push_req(aio_work_t *work,aio_work_node_t *work_node,int mask)
{
	logDebug("aio_push_req,%s,size %d",work_node->file_name,work_node->size);
	work_node->next = NULL;
	aio_work_queue_t *req_queue;
	req_queue = work->req_queue;

	LOCK_IF_ERROR("aio_push_req",req_queue->lock);
	req_queue->tail->next = work_node;
	req_queue->tail = work_node;
	work_node->status = mask;
		
	if(req_queue->head->next == work_node)
		pthread_cond_signal(&work->req_cond);
	UNLOCK_IF_ERROR("aio_push_req",req_queue->lock);
	return 0;
}

int aio_push_res(aio_work_node_t *work_node)
{
	logDebug("aio_push_res,%s,size %d",work_node->file_name,work_node->size);
	work_node->next = NULL;
	aio_work_queue_t *res_queue;
	res_queue = work_node->res_queue;

	LOCK_IF_ERROR("aio_push_res",res_queue->lock);
	res_queue->tail->next = work_node;
	res_queue->tail = work_node;
		
	if(res_queue->head->next == work_node)
	{
		if(write(res_queue->pipe_fd[1],&res_queue->dummy,1) == -1)
		{
			/*nothing to do,just to avoid the warning..*/
		}
	}
	UNLOCK_IF_ERROR("aio_push_res",res_queue->lock);
	return 0;
}

aio_work_node_t* aio_pop_req(aio_work_queue_t *req_queue)
{
	logDebug("aio_pop_req",...);
	aio_work_node_t* temp_work_node;
		
	LOCK_IF_ERROR("aio_pop_req",req_queue->lock);
	temp_work_node = req_queue->head->next;
	if(temp_work_node == req_queue->tail)
	{
		req_queue->tail = req_queue->head;
	}
	req_queue->head->next = temp_work_node->next;
	UNLOCK_IF_ERROR("aio_pop_req",req_queue->lock);
	return temp_work_node;
}

aio_work_node_t* aio_pop_res(aio_work_queue_t *res_queue)
{
	logDebug("aio_pop_res",...);
	aio_work_node_t* temp_work_node;
		
	LOCK_IF_ERROR("aio_push_res",res_queue->lock);
	temp_work_node = res_queue->head->next;
	if(temp_work_node == res_queue->tail)
	{
		res_queue->tail = res_queue->head;
	}
	res_queue->head->next = temp_work_node->next;
	UNLOCK_IF_ERROR("aio_push_res",res_queue->lock);
	return temp_work_node;
}

void aio_process_res_queue(aio_work_queue_t *res_queue,void *eventLoop)
{
	aio_work_node_t *work_node;
	logDebug("aio_process_res_queue",...);

	LOCK_IF_ERROR("aio_process_res_queue",res_queue->lock);
	while(res_queue->head->next)
	{
		UNLOCK_IF_ERROR("aio_process_res_queue",res_queue->lock);
		work_node = aio_pop_res(res_queue);
		work_node->proc(work_node);
		LOCK_IF_ERROR("aio_process_res_queue",res_queue->lock);
	}
	UNLOCK_IF_ERROR("aio_process_res_queue",res_queue->lock);
	
	return ;
}

void *aio_process_req_queue(void *arg)
{
	aio_work_t *work;
	aio_work_queue_t *req_queue;
	aio_work_node_t *work_node;

	work = (aio_work_t *)arg;

	req_queue = work->req_queue;

	while(work->stop)
	{
		logDebug("aio_process_req_queue",...);
		LOCK_IF_ERROR("aio_process_req_queue",req_queue->lock);
		if(req_queue->head->next == NULL)
		{
			pthread_cond_wait(&work->req_cond,&req_queue->lock);
		}
		UNLOCK_IF_ERROR("aio_process_req_queue",req_queue->lock);

		work_node = aio_pop_req(req_queue);
		if(work_node->status == AIO_READ)
			work_node->size = read(work_node->fd,work_node->buf,work_node->size);
		else
			work_node->size = write(work_node->fd,work_node->buf,work_node->size);
		aio_push_res(work_node);
	}
	return NULL;
}

int aio_main(aio_work_t *work)
{
	work->stop = 1;

	pthread_cond_init(&work->req_cond,NULL);
	
	aio_create_work_queue(work);

	if(pthread_create(&work->p_tid_aio, NULL,aio_process_req_queue,work) != 0)
	{
		logError(	"file :"__FILE__",line :%d"\
			"aioMain pthread_create aioProcessWorkQueue failed,"\
			"errno :%d,error info: %s",\
			__LINE__,errno,strerror(errno));
		return -2;
	}
	return 0;
}

int aio_delete_work_queue(aio_work_t *work)
{
	work->stop = 0;
	pthread_cond_destroy(&work->req_cond);
	pthread_mutex_destroy(&work->wait_lock);
	pthread_mutex_destroy(&work->req_queue->lock);
	free(work->req_queue);
	return 0;
}
