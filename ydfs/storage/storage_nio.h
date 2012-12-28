#ifndef STORAGE_NIO_H
#define STORAGE_NIO_H

#define EACH_MEM_POOL_SIZE 1024

#include <pthread.h>
#include "aio.h"
#include "trunk_file.h"
#include "mem_pool.h"

struct aeEventLoop;

struct AIO_WORK_QUEUE_T;

typedef struct NIO_NODE_T
{
	int nio_node_id;
	int pipe_fd[2];	
	/*for aio_pool pop back*/
	struct AIO_WORK_QUEUE_T *res_queue;
	trunk_file_node_t *non_occupied_trunk_file_head;
	mem_pool_t dio_buf_pool;
	mem_pool_t pClient_buf_pool;
	
	struct aeEventLoop *eventLoop;
}nio_node_t;

typedef struct NIO_POOL_T
{
	int max_thread_num;
	pthread_t *threadid;
	nio_node_t *nio_node;
}nio_pool_t;

extern nio_pool_t *g_nio_pool;

int init_aio_and_nio_node(nio_node_t *nio_node);

int init_nio_pool(nio_pool_t *nio_pool);

#endif
