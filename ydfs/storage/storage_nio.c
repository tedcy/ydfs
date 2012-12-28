#include "storage_nio.h"
#include "storage_net.h"
#include "ae.h"
#include "log.h"

static int init_nio_node_pipe(nio_node_t *nio_node);
static int init_aio_node_pipe(nio_node_t *nio_node);
static void *work_thread_entrance(void *arg);
static void process_res_queue(struct aeEventLoop *eventLoop, int pipe_fd, void *clientData, int mask);
static void create_aio_event(nio_node_t *nio_node);
static nio_node_t* create_worker(nio_node_t *nio_node);
static void run_worker(nio_node_t *nio_node);

int init_nio_pool(nio_pool_t *nio_pool)
{
	int i;
	
	nio_node_t *nio_node;

	nio_pool->threadid = (pthread_t*)malloc(nio_pool->max_thread_num*sizeof(pthread_t));
	nio_pool->nio_node = (nio_node_t*)malloc(nio_pool->max_thread_num*sizeof(nio_node_t));
	for(i = 0;i != nio_pool->max_thread_num;++i)
	{
		nio_node = nio_pool->nio_node + i;
		nio_node->nio_node_id = i;
		nio_node->dio_buf_pool.max_node_num = EACH_MEM_POOL_SIZE;
		nio_node->dio_buf_pool.node_size = sizeof(aio_work_node_t);
		nio_node->pClient_buf_pool.max_node_num = EACH_MEM_POOL_SIZE;
		nio_node->pClient_buf_pool.node_size = sizeof(storage_client_t);
		if(init_mem_pool(&nio_node->dio_buf_pool) < 0)
			return -1;

		if(init_mem_pool(&nio_node->pClient_buf_pool) < 0)
			return -1;

		pthread_create(&nio_pool->threadid[i],NULL,work_thread_entrance,nio_node);
	}
	return 0;
}

static int init_nio_node_pipe(nio_node_t *nio_node)
{
	if(pipe(nio_node->pipe_fd) < 0)
	{
		logError(	"file :"__FILE__",line :%d"\
			"init_nio_pool_pipe call pipe failed.",\
			__LINE__);
		return -1;
	}
	return 0;
}

static int init_aio_node_pipe(nio_node_t *nio_node)
{
	if(pipe(nio_node->res_queue->pipe_fd) < 0)
	{
		logError(	"file :"__FILE__",line :%d"\
			"init_nio_node_pipe call pipe failed.",\
			__LINE__);
		return -1;
	}
	return 0;
}

static void *work_thread_entrance(void *arg)
{
	nio_node_t *nio_node;

	nio_node = (nio_node_t*)arg;
	
	init_nio_node_pipe(nio_node);

	if((nio_node = create_worker(nio_node)) == NULL)
		return NULL;
	
	run_worker(nio_node);
	return NULL;
}	

static void process_res_queue(struct aeEventLoop *eventLoop, int pipe_fd, void *clientData, int mask)
{
	nio_node_t *nio_node;
	nio_node = (nio_node_t *)clientData;
	if(read(pipe_fd,&nio_node->res_queue->dummy,1) == -1)
	{
		/*do nothing,just to avoid the warning*/
	}
	aio_process_res_queue(nio_node->res_queue,eventLoop);
	return ;
}

static void create_aio_event(nio_node_t *nio_node)
{
	if(aeCreateFileEvent(nio_node->eventLoop, nio_node->res_queue->pipe_fd[0], AE_READABLE,process_res_queue,nio_node) != AE_OK)
	{
		logError(	"file :"__FILE__",line :%d"\
			"create_aio_event CreateFileEvent failed.");
		exit(1);
	}
}

int init_aio_and_nio_node(nio_node_t *nio_node)
{
	aio_create_res_queue(nio_node);
	
	init_aio_node_pipe(nio_node);

	create_aio_event(nio_node);
	
	return 0;
}

static nio_node_t* create_worker(nio_node_t *nio_node)
{
	if((nio_node->eventLoop = (aeEventLoop*)aeCreateEventLoop()) == NULL)
	{
		logError(	"file :"__FILE__",line :%d"\
			"create_tracker_service CreateEventLoop failed.");
			return NULL;
	}

	init_aio_and_nio_node(nio_node);
	
	if(aeCreateFileEvent(nio_node->eventLoop,nio_node->pipe_fd[0], AE_READABLE,one_work_start,nio_node) != AE_OK)
	{
		logError(	"file :"__FILE__",line :%d"\
			"create_tracker_service CreateFileEvent failed.");
			return NULL;
	}

	return nio_node;
}

static void run_worker(nio_node_t *nio_node)
{
	aeEventLoop *eventLoop = nio_node->eventLoop;

	aeSetBeforeSleepProc(eventLoop,NULL);
	aeMain(eventLoop);
	aeDeleteEventLoop(eventLoop);
	
	close(nio_node->pipe_fd[0]);
	close(nio_node->pipe_fd[1]);
}
