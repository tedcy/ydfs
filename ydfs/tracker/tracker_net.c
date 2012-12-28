#include "tracker_nio.h"
#include "tracker_net.h"
#include "tracker_service.h"

static void one_work_start(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);

tracker_client_t* init_tracker_client(nio_node_t *nio_node)
{
	tracker_client_t *pClient;
	mem_node_t *mem_node;
	
	mem_node = get_freed_mem(&nio_node->pClient_buf_pool);
	if(mem_node == NULL)
		return NULL;
		
	pClient = mem_node->buf;
	memset(pClient,0,(sizeof(*pClient)));

	pClient->nio_node = nio_node;
	pClient->mem_node = mem_node;
	
	return pClient;
}

void clean_tracker_client(tracker_client_t *pClient)
{
	if(pClient == NULL)
		return ;
	if(pClient->sockfd != 0)
		close(pClient->sockfd);
	release_mem(&pClient->nio_node->pClient_buf_pool,pClient->mem_node);
	return;
}

void refresh_tracker_client_data(tracker_client_t *pClient)
{
	if(pClient == NULL)
		return ;
	memset(&pClient->data,0,sizeof(pClient->data));
}

void finish_tracker_CLOSE_WAIT_stat(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	int result;
	while(1)
	{
		result = read(sockfd,&g_tracker_service.FINISH_CLOSE_WAIT_BUFF,1024);
		if(result == 0||(result == -1&&errno != EAGAIN))
			break;
	}

	aeDeleteFileEvent(eventLoop,sockfd,AE_READABLE);
	clean_tracker_client((tracker_client_t*)clientData);
	return ;
}

int init_tracker_service()
{
	g_tracker_service.nio_pool.one_work_start = one_work_start;	

	init_nio_pool(&g_tracker_service.nio_pool);
	init_storage_pool();
	return 0;
}

int create_tracker_service()
{
	if((g_tracker_service.sockfd = sock_server(g_tracker_service.server_port)) < 0)
		return -1;
	
	if (tcpsetserveropt(g_tracker_service.sockfd, g_tracker_service.network_timeout) != 0)
	{
		return -2;
	}
	
	if((g_tracker_service.eventLoop = (aeEventLoop*)aeCreateEventLoop()) == NULL)
	{
		logError(	"file :"__FILE__",line :%d"\
			"create_tracker_service CreateEventLoop failed.");
			return -3;
	}

	aeCreateTimeEvent(g_tracker_service.eventLoop,g_tracker_service.check_storage_pool_millisec,check_storage_pool,NULL,NULL);
	
	if(aeCreateFileEvent(g_tracker_service.eventLoop, g_tracker_service.sockfd, AE_READABLE,accept_tcp,NULL) != AE_OK)
	{
		logError(	"file :"__FILE__",line :%d"\
			"create_tracker_service CreateFileEvent failed.");
			return -4;
	}
	return 0;
}

int run_tracker_service()
{
	aeSetBeforeSleepProc(g_tracker_service.eventLoop,NULL);
	aeMain(g_tracker_service.eventLoop);
	aeDeleteEventLoop(g_tracker_service.eventLoop);
	
	close(g_tracker_service.sockfd);
	return 0;
}

void accept_tcp(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
	int sockfd;
	struct sockaddr_in remote_addr;
	socklen_t sin_size;
	nio_pool_t *nio_pool;
	
	sin_size = sizeof(struct sockaddr);
	
	if((sockfd = accept(fd,(struct sockaddr*)&remote_addr,&sin_size)) < 0)
	{
		logError(	"file :"__FILE__",line :%d"\
			"accept_tcp accept failed,"\
			"errno :%d,error info: %s",\
			__LINE__,errno,strerror(errno));
		exit(1);
	}

	nio_pool = &g_tracker_service.nio_pool;
	if(write(nio_pool->nio_node[sockfd % nio_pool->max_thread_num].pipe_fd[1],\
		&sockfd,sizeof(sockfd)) != sizeof(sockfd))
	{
		logError(	"file :"__FILE__",line :%d"\
			"accept_tcp put incomesock failed.",\
			__LINE__);
		return ;
	}
	return ;
}

static void one_work_start(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
	int sockfd;
		
	if(read(fd,&sockfd,sizeof(sockfd)) != sizeof(sockfd))
	{
		logError(	"file :"__FILE__",line :%d"\
			"one_work_start get incomesock failed.",\
			__LINE__);
		return ;
	}
	
	if(setnonblocking(sockfd) < 0)
	{
		logError(	"file :"__FILE__",line :%d"\
			"one_work_start set incomesock nonblocking failed.",\
			__LINE__);
		return ;
	}
	
	tracker_client_t *pClient;

	pClient = init_tracker_client(clientData);
	
	pClient->sockfd = sockfd;
	
	tracker_nio_read(&pClient->cmd,sizeof(pClient->cmd),accept_command,heart_beat_stop,"one_work_start");
	
	return ;
}
