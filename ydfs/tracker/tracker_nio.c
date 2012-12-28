#include "tracker_nio.h"
#include "tracker_global.h"
#include "ae.h"
#include "log.h"

static void *work_thread_entrance(void *arg);
static nio_node_t* create_worker(nio_node_t *nio_node);
static void run_worker(nio_node_t *nio_node);
static int init_nio_node_pipe(nio_node_t *nio_node);

nio_pool_t *g_nio_pool;

int init_nio_pool(nio_pool_t *nio_pool)
{
	int i;
	
	g_nio_pool = nio_pool;
	
	nio_pool->threadid = (pthread_t*)malloc(nio_pool->max_thread_num*sizeof(pthread_t));
	nio_pool->nio_node = (nio_node_t*)malloc(nio_pool->max_thread_num*sizeof(nio_node_t));
	for(i = 0;i != nio_pool->max_thread_num;++i)
	{
		nio_pool->nio_node[i].nio_node_id = i;
		nio_pool->nio_node[i].pClient_buf_pool.max_node_num = 1000;
		nio_pool->nio_node[i].pClient_buf_pool.node_size = sizeof(tracker_client_t);
		pthread_create(&nio_pool->threadid[i],NULL,work_thread_entrance,nio_pool->nio_node+i);
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

static void *work_thread_entrance(void *arg)
{
	nio_node_t *nio_node;

	nio_node = (nio_node_t*)arg;

	init_nio_node_pipe(nio_node);

	if(init_mem_pool(&nio_node->pClient_buf_pool) < 0)
		return NULL;

	if((nio_node = create_worker(nio_node)) == NULL)
		return NULL;
	
	run_worker(nio_node);
	return NULL;
}	

static nio_node_t* create_worker(nio_node_t *nio_node)
{
	if((nio_node->eventLoop = (aeEventLoop*)aeCreateEventLoop()) == NULL)
	{
		logError(	"file :"__FILE__",line :%d"\
			"create_tracker_service CreateEventLoop failed.");
			return NULL;
	}
	
	if(aeCreateFileEvent(nio_node->eventLoop,nio_node->pipe_fd[0], AE_READABLE,g_nio_pool->one_work_start,nio_node) != AE_OK)
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
