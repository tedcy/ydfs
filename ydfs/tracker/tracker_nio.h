#ifndef TRACKER_NIO_H
#define TRACKER_NIO_H

#include <pthread.h>

#include "mem_pool.h"

struct aeEventLoop;

typedef struct NIO_NODE_T
{
	int nio_node_id;
	int pipe_fd[2];	
	mem_pool_t pClient_buf_pool;

	struct aeEventLoop *eventLoop;
}nio_node_t;

typedef struct NIO_POOL_T
{
	int max_thread_num;
	pthread_t *threadid;
	nio_node_t *nio_node;
	void (*one_work_start)(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
}nio_pool_t;

extern nio_pool_t *g_nio_pool;

int init_nio_pool(nio_pool_t *nio_pool);

#endif
