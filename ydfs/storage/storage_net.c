#include <pthread.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#include "storage_nio.h"
#include "storage_sync.h"
#include "storage_net.h"
#include "storage_service.h"
#include "base.h"
#include "share_fun.h"
#include "debug.h"

static void do_new_nb_sock_send_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);

/*nio_send_file*/
/*
static void finish_buf_send(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
static void do_send_file_count(aio_work_node_t *work_node);
*/
/*nio_send_file end*/

/*nio_recv_file*/
static void do_recv_file_count(aio_work_node_t *work_node);
static void do_recv_file_count_end(aio_work_node_t *work_node);
static void finish_buf_read(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
/*nio_recv_file end*/

/*aio*/
static int create_aio_pool();
/*aio end*/

storage_client_t* init_storage_client(nio_node_t *nio_node)
{
	storage_client_t *pClient;
	mem_node_t *mem_node;
	
	mem_node = get_freed_mem(&nio_node->pClient_buf_pool);
	if(mem_node == NULL)
		return NULL;
		
	pClient = mem_node->buf;
	memset(pClient,0,(sizeof(*pClient)));

	pClient->nio_node = nio_node;
	pClient->mem_node = mem_node;
	pClient->file.file_size = -1;
	return pClient;
}

void clean_storage_client(storage_client_t *pClient)
{
	fflush(g_storage_service.sync_pool.binlog_fp);
	
	if(pClient == NULL)
		return ;
	if(pClient->sockfd != 0)
		close(pClient->sockfd);
	
	release_mem(&pClient->nio_node->pClient_buf_pool,pClient->mem_node);
	return;
}

void refresh_storage_client_data(storage_client_t *pClient)
{
	if(pClient == NULL)
		return ;
	memset(&pClient->data,0,sizeof(pClient->data));
	return ;
}

void refresh_storage_client_file(storage_client_t *pClient)
{
	if(pClient == NULL)
		return ;
	memset(&pClient->file,0,sizeof(pClient->file));
	pClient->file.file_size = -1;
	return ;
}

void finish_storage_CLOSE_WAIT_stat(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	int result;
	while(1)
	{
		result = read(sockfd,&g_storage_service.FINISH_CLOSE_WAIT_BUFF,1024);
		if(result == 0||(result == -1&&errno != EAGAIN))
			break;
	}

	aeDeleteFileEvent(eventLoop,sockfd,AE_READABLE);
	clean_storage_client((storage_client_t*)clientData);
	return ;
}

static void do_new_nb_sock_send_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	storage_client_t *pClient;
	pClient = clientData;
	int size;
	
	if((size = sendfile(sockfd,pClient->file.fd,&pClient->file.start_offlen,\
		pClient->file.file_size-pClient->file.read_size)) == -1&&errno != EAGAIN)
	{
		logError(	"file: "__FILE__",line :%d, "\
			"sock_send_file sendfile failed,"\
			"errno: %d,error info: %s",\
			__LINE__,errno,strerror(errno));
		return ;
	}
	pClient->file.read_size += size;

	if(pClient->file.read_size == pClient->file.file_size)
	{
		aeDeleteFileEvent(eventLoop,sockfd,AE_WRITABLE);
		if(pClient->file.proc != NULL)
			pClient->file.proc(eventLoop,sockfd,pClient,AE_WRITABLE);
	}
	return ;
}

void new_nb_sock_send_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	storage_client_t *pClient;

	pClient = clientData;
	logDebug("new_nb_sock_send_file %s,%d",pClient->file.file_name,pClient->file.start_offlen);

	if(pClient->file.fd == 0)
	{
		if((pClient->file.fd = open(pClient->file.file_name,O_RDONLY)) == -1)
		{
			logError(	"file: "__FILE__",line: %d,"\
				"new_nb_sock_send_file call open file failed,"\
				"errno: %d,error info: %s",\
				__LINE__,errno,strerror(errno));
			return ;
		}
	}
	if(aeCreateFileEvent(eventLoop,sockfd, AE_WRITABLE,do_new_nb_sock_send_file,pClient) != AE_OK)
	{
			logError(	"file :"__FILE__",line :%d\
				new_nb_sock_send_file CreateFileEvent failed.",\
				__LINE__);
			return ;
	}
	return ;
}
/*nio_send_file*/
/*out of service*/
/*
static void finish_buf_send(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	storage_client_t *pClient;
	aio_work_node_t *work_node;
	
	pClient = clientData;
	work_node = pClient->cur_aio_node;
	
	pClient->file.writ_size += pClient->data.total_size;
	
	logDebug("file_name %s,buf total send_size %d,buf want total size %d\n",pClient->file.file_name,pClient->file.writ_size,work_node->size);
	
	if(pClient->file.read_size != pClient->file.file_size)
	{
		nb_sock_send_file(eventLoop,sockfd,pClient,AE_WRITABLE);
		return ;
	}
	
	free(work_node->buf);
	free(work_node);
	close(pClient->file.fd);
	pClient->file.proc(eventLoop,sockfd,pClient,AE_WRITABLE);
}

static void do_send_file_count(aio_work_node_t *work_node)
{
	storage_client_t *pClient;
	aeEventLoop *eventLoop;
	int sockfd;

	pClient = work_node->arg;
	sockfd = pClient->sockfd;
	eventLoop = pClient->nio_node->eventLoop;
	logDebug("do_send_file_count %s %d",pClient->file.file_name,work_node->size);
	
	storage_nio_write(work_node->buf,work_node->size,finish_buf_send,NULL,"do_send_file_count");
	
	return ;
}

void nb_sock_send_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	aio_work_node_t *work_node;
	storage_client_t *pClient;
	
	pClient = clientData;
		
	logDebug("nb_sock_send_file %s,%d",pClient->file.file_name,pClient->file.start_offlen);
	
	if(pClient->file.fd == 0)
	{
		if((pClient->file.fd = open(pClient->file.file_name,O_RDONLY)) == -1)
		{
			logError(	"file: "__FILE__",line: %d,"\
				"nb_sock_send_file call open file failed,"\
				"errno: %d,error info: %s",\
				__LINE__,errno,strerror(errno));		
			clean_storage_client(pClient);
			return ;
		}
		lseek(pClient->file.fd,pClient->file.start_offlen,SEEK_SET);
		pClient->cur_aio_node = malloc(sizeof(aio_work_node_t));
		pClient->cur_aio_node->buf = malloc(BUF_MAXSIZE);
//
		pClient->cur_aio_node->file_name = pClient->file.file_name;
//
		pClient->cur_aio_node->fd = pClient->file.fd;
		pClient->cur_aio_node->proc = do_send_file_count;
		pClient->cur_aio_node->arg = pClient;
		pClient->cur_aio_node->res_queue = pClient->nio_node->res_queue;
	}
	work_node = pClient->cur_aio_node;

	if(pClient->file.read_size+BUF_MAXSIZE <= pClient->file.file_size)
	{
		work_node->size = BUF_MAXSIZE;
	}
	else
	{
		work_node->size = pClient->file.file_size-pClient->file.read_size;
	}
	pClient->file.read_size += work_node->size;
	logDebug("%d %d",pClient->file.read_size,work_node->size);
	aio_push_req(&g_storage_service.aio_work,work_node,AIO_READ);
	
	return ;
}
*/
/*nio_send_file end*/

/*nio_recv_file*/
static void do_recv_file_count(aio_work_node_t *work_node)
{
	storage_client_t *pClient;
	
	pClient = (storage_client_t *)work_node->arg;
	
	pClient->file.writ_size += work_node->size;

	if(pClient->trunk_file_node != NULL)
	{
		pClient->trunk_file_node->size += work_node->size;	
	}

	logDebug("file_name %s,now_write_size %d,total writ %d",pClient->file.file_name,work_node->size,pClient->file.writ_size);
	
	release_mem(&pClient->nio_node->dio_buf_pool,work_node->mem_node);
	return ;
}

static void do_recv_file_count_end(aio_work_node_t *work_node)
{
	storage_client_t *pClient;
	
	pClient = (storage_client_t *)work_node->arg;
	
	pClient->file.writ_size += work_node->size;

	if(pClient->trunk_file_node != NULL)
	{
		pClient->trunk_file_node->size += work_node->size;	
		release_occupied_trunk_file(pClient->nio_node->non_occupied_trunk_file_head,pClient->trunk_file_node);
	}
	else
	{
		/*synced file*/
		close(pClient->file.fd);
	}
	logDebug("file_name %s,now_write_size %d,total writ %d",pClient->file.file_name,work_node->size,pClient->file.writ_size);
	
	release_mem(&pClient->nio_node->dio_buf_pool,work_node->mem_node);
	
	if(pClient->file.proc != NULL)
		pClient->file.proc(pClient->nio_node->eventLoop,pClient->sockfd,pClient,AE_READABLE);
	
	return ;
}

static void finish_buf_read(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	storage_client_t *pClient;
	aio_work_node_t *work_node;
	
	pClient = (storage_client_t *)clientData;
	work_node = pClient->cur_aio_node;
	
	pClient->file.read_size += pClient->data.total_size;

	work_node->fd = pClient->file.fd;
	work_node->file_name = pClient->file.file_name;
		
	work_node->size = pClient->data.total_size;
		
	logDebug("file_name %s,now_recv_count %d,total recv_size %d,want recv_size %d",pClient->file.file_name,pClient->data.total_size,pClient->file.read_size,pClient->file.file_size);
	
	aio_push_req(&g_storage_service.aio_work,work_node,AIO_WRIT);

	nb_sock_recv_file(eventLoop,sockfd,pClient,AE_NONE);

	return ;
}

void nb_sock_recv_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	aio_work_node_t *work_node;
	storage_client_t *pClient;
	nb_file_t *file;
	char temp_storage_id;
	
	pClient = (storage_client_t *)clientData;
	file = &pClient->file;
		
	logDebug("nb_sock_recv_file,%s",pClient->file.file_name);

	if(file->need_name == 1)
	{
//		
		pClient->trunk_file_node = grasp_non_occupied_trunk_file(&g_storage_service.trunk_file_pool,\
			pClient->nio_node->non_occupied_trunk_file_head,pClient->file.file_size);
	
		LOCK_IF_ERROR("nb_sock_recv_file",g_storage_service.heart_beat_lock);
		int_to_string(g_storage_service.heart_beat.storage_id,&temp_storage_id,1);
		UNLOCK_IF_ERROR("nb_sock_recv_file",g_storage_service.heart_beat_lock);
		pClient->file.start_offlen = pClient->trunk_file_node->size;
		pClient->trunk_file_node->file_name[4] = temp_storage_id;
		memcpy(pClient->file.file_name+8,pClient->trunk_file_node->file_name,5);

		if(pClient->trunk_file_node->fd == 0)
		{
			if((pClient->trunk_file_node->fd = open(pClient->trunk_file_node->file_name,O_WRONLY|O_CREAT,0664)) == -1)
			{
				logError(	"file: "__FILE__",line: %d,"\
					"nb_sock_recv_file call open file failed,"\
					"errno: %d,error info: %s",\
					__LINE__,errno,strerror(errno));		
				clean_storage_client(pClient);
				return ;
			}
		}
		pClient->file.fd = pClient->trunk_file_node->fd;
//
		storage_nio_read(&file->file_size,sizeof(file->file_size),do_upload_to_storage,NULL,"nb_sock_recv_file");

		return ;
	}
		
	mem_node_t *mem_node;
	if(file->read_size+BUF_MAXSIZE <= file->file_size)
	{
		mem_node = get_freed_mem(&pClient->nio_node->dio_buf_pool);
		if(mem_node == NULL)
			return ;
		
		work_node = mem_node->buf;
		work_node->mem_node = mem_node;

		pClient->cur_aio_node = work_node;

		work_node->arg = pClient;
		work_node->proc = do_recv_file_count;
		work_node->res_queue = pClient->nio_node->res_queue;

		storage_nio_read(&work_node->buf,BUF_MAXSIZE,finish_buf_read,NULL,"nb_sock_recv_file");
		
		return ;
	}
	if(file->read_size != file->file_size)
	{
		mem_node = get_freed_mem(&pClient->nio_node->dio_buf_pool);
		if(mem_node == NULL)
			return ;
		
		work_node = mem_node->buf;
		work_node->mem_node = mem_node;

		pClient->cur_aio_node = work_node;

		work_node->arg = pClient;
		work_node->proc = do_recv_file_count_end;
		work_node->res_queue = pClient->nio_node->res_queue;

		storage_nio_read(&work_node->buf,file->file_size-file->read_size,finish_buf_read,NULL,"nb_sock_recv_file");

		return ;
	}
	return ;
}
/*nio_recv_file end*/

static int create_aio_pool()
{
	if(aio_main(&g_storage_service.aio_work) < 0)
		return -1;

	return 0;
}

int init_storage_service()
{
	if(create_aio_pool() < 0)
		return -1;
	
	pthread_mutex_init(&g_storage_service.heart_beat_lock,NULL);
	init_trunk_file_pool(&g_storage_service.trunk_file_pool);
	init_nio_pool(&g_storage_service.nio_pool);
	init_sync_pool(&g_storage_service.sync_pool,&g_storage_service.heart_beat.count_stamp,g_storage_service.trunk_file_pool.real_file_name);
	return 0;
}

int create_storage_service()
{
	if((g_storage_service.sockfd = sock_server(g_storage_service.heart_beat.port)) < 0)
		return -1;
	
	if (tcpsetserveropt(g_storage_service.sockfd, g_storage_service.network_timeout) != 0)
	{
		return -2;
	}
	
	if((g_storage_service.eventLoop = (aeEventLoop*)aeCreateEventLoop()) == NULL)
	{
		logError(	"file :"__FILE__",line :%d"\
			"create_storage_service CreateEventLoop failed.");
		exit(1);
	}

	aeCreateTimeEvent(g_storage_service.eventLoop,g_storage_service.heart_beat_millisec,heart_beat_init,g_storage_service.nio_pool.nio_node,NULL);

	if(aeCreateFileEvent(g_storage_service.eventLoop, g_storage_service.sockfd, AE_READABLE,accept_tcp,NULL) != AE_OK)
	{
		logError(	"file :"__FILE__",line :%d"\
			"create_storage_service CreateFileEvent failed.");
		exit(1);
	}
	return 0;
}

int run_storage_service()
{
	aeSetBeforeSleepProc(g_storage_service.eventLoop,NULL);
	aeMain(g_storage_service.eventLoop);
	aeDeleteEventLoop(g_storage_service.eventLoop);
	
	close(g_storage_service.sockfd);
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

	nio_pool = &g_storage_service.nio_pool;
	//write to different sockfd as random
	if(write(nio_pool->nio_node[sockfd % nio_pool->max_thread_num].pipe_fd[1],&sockfd,sizeof(sockfd)) != sizeof(sockfd))
	{
		logError(	"file :"__FILE__",line :%d"\
			"accept_tcp put incomesock failed.",\
			__LINE__);
		return ;
	}
	return ;
}

void one_work_start(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
	int incomesock;
		
	if(read(fd,&incomesock,sizeof(incomesock)) != sizeof(incomesock))
	{
		logError(	"file :"__FILE__",line :%d"\
			"one_work_start get incomesock failed.",\
			__LINE__);
		return ;
	}
	
	if(setnonblocking(incomesock) < 0)
		return ;
	
	storage_client_t *pClient;

	pClient = init_storage_client(clientData);

	pClient->sockfd = incomesock;
	
	pClient->data.buff = &pClient->cmd;
	pClient->data.need_size = sizeof(pClient->cmd);
	pClient->data.proc = accept_command;
	
	if(aeCreateFileEvent(eventLoop, incomesock, AE_READABLE,nb_sock_recv_data,pClient) != AE_OK)
	{
		logError(	"file :"__FILE__",line :%d"\
			"one_work_start CreateFileEvent failed.",\
			__LINE__);
		clean_storage_client(pClient);
		return ;
	}
	return ;
}
