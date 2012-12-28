#include <arpa/inet.h>
#include <fcntl.h>

#include "storage_sync.h"
#include "storage_recovery.h"
#include "storage_proto.h"
#include "storage_net.h"
#include "base.h"
#include "ae.h"
#include "log.h"

static int init_stop_sync_pipe(sync_node_t *sync_node);
static void sync_stop(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
/*sync_recv_file*/
/*
**we first recv the file name,then call nb_recv_file
*/
static void finish_recv_sync_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
static void finish_sync_recv_file_name(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
/*sync_recv_file end*/

/*sync_send_file*/
/*
**we first send the file name,the synced storage get it,and recv at the start_len in trunk_name
**in this way,client can download the file which is synced
*/
static void finish_send_sync_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
static void sync_send_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
static void sync_send_file_name(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
/*sync_send_file end*/

/*sync_work*/
/*
**when the thread work,we connect to the other storage by storage_id,
**we create connect time event to avoid connecting error,we must try again
**then create check_count_stamp event to check,the check will be one by one,
**if someting wrong,must create it again
*/
static int check_count_stamp(struct aeEventLoop *eventLoop, long long id, void *clientData);
static void start_check_count_stamp(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
static void restart_sync_connect(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
/*sync_work end*/

static aeEventLoop* create_sync_worker(aeEventLoop *eventLoop,sync_node_t *sync_node);
static void run_sync_worker(aeEventLoop *eventLoop);
static void* new_sync_thread_entrance(void *arg);

static int init_stop_sync_pipe(sync_node_t *sync_node)
{
	if(pipe(sync_node->stop_fd) < 0)
	{
		logError(	"file :"__FILE__",line :%d"\
			"init_stop_sync_pipe call pipe failed.",\
			__LINE__);
		return -1;
	}
	return 0;
}

static void sync_stop(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	char dummy;
	if(read(sockfd,&dummy,1) == -1)
	{
		/*do nothing,just to avoid the warning*/
	}
	pthread_exit(NULL);	
	return ;
}
int init_sync_pool(sync_pool_t *sync_pool,count_stamp_t *count_stamp,char *real_count_stamp_file_name)
{
	int storage_iter;
	sync_node_t *sync_node;
	nio_node_t *nio_node;

	sync_pool->count_stamp = count_stamp;
	sync_pool->real_count_stamp_file_name = real_count_stamp_file_name;
	sync_pool->binlog_fp = fopen(BINLOG_NAME,"a+");
	sync_pool->synclog_fp = fopen(SYNCLOG_NAME,"a+");

	pthread_mutex_init(&sync_pool->binlog_read_count_lock,NULL);
	pthread_mutex_init(&sync_pool->synclog_read_count_lock,NULL);

	for(storage_iter = 0;storage_iter != STORAGE_MAXSIZE;++storage_iter)
	{
		sync_node = sync_pool->sync_node + storage_iter;
		sync_node->storage_id = storage_iter;
		sync_node->sync_pool = sync_pool;
		memcpy(sync_pool->count_stamp->storage[storage_iter],"////////",8);
	
		nio_node = &sync_node->nio_node;

		nio_node->dio_buf_pool.max_node_num = 10;
		nio_node->dio_buf_pool.node_size = sizeof(aio_work_node_t);
		nio_node->pClient_buf_pool.max_node_num = 10;
		nio_node->pClient_buf_pool.node_size = sizeof(storage_client_t);
		if(init_mem_pool(&nio_node->dio_buf_pool) < 0)
			return -1;

		if(init_mem_pool(&nio_node->pClient_buf_pool) < 0)
			return -1;
	}	
	return 0;
}
/*sync_recv_file*/
static void finish_recv_sync_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	storage_client_t *pClient;
	pClient = clientData;
	int storage_id;

	logDebug("finish_recv_sync_file %s",pClient->file.file_name);

	LOCK_IF_ERROR("finish_recv_sync_file",g_storage_service.heart_beat_lock);
	g_storage_service.heart_beat.storage_size += pClient->file.read_size;
	UNLOCK_IF_ERROR("finish_recv_sync_file",g_storage_service.heart_beat_lock);

	storage_id = string_to_int(pClient->file.real_file_name + 12,1);

	memcpy(g_storage_service.sync_pool.count_stamp->storage[storage_id],pClient->file.real_file_name,8);

	LOCK_IF_ERROR("finish_recv_sync_file",g_storage_service.sync_pool.synclog_read_count_lock);
	write_file_log(pClient->file.real_file_name,24,g_storage_service.sync_pool.synclog_fp);
	UNLOCK_IF_ERROR("finish_recv_sync_file",g_storage_service.sync_pool.synclog_read_count_lock);


	refresh_storage_client_file(clientData);
	do_sync_from_storage(eventLoop,sockfd,clientData,AE_NONE);
	return ;	
}

static void finish_sync_recv_file_name(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	storage_client_t *pClient;
	pClient = clientData;
	
	pClient->file.proc = finish_recv_sync_file;

	pClient->file.start_offlen = string_to_int(pClient->file.file_name + 15,4);
	pClient->file.file_size = string_to_int(pClient->file.file_name + 19,4);

	memcpy(pClient->file.real_file_name,pClient->file.file_name,24);

	logDebug("finish_sync_recv_file_name %s",pClient->file.file_name);
	/*get all_trunk_name*/	
	memcpy(pClient->file.file_name,pClient->file.file_name+8,5);
	pClient->file.file_name[5] = '\0';
	logDebug("finish_sync_recv_trunk_name %d,%d,%s",pClient->file.file_size,pClient->file.start_offlen,pClient->file.file_name);

	if((pClient->file.fd = open(pClient->file.file_name,O_WRONLY|O_CREAT,0664)) == -1)
	{
		logError(	"file: "__FILE__",line :%d, "\
			"finish_sync_recv_file_name open file failed,"\
			"errno: %d,error info: %s",\
			__LINE__,errno,strerror(errno));
		*(char *)NULL = 0;
	}
	lseek(pClient->file.fd,pClient->file.start_offlen,SEEK_SET);

	nb_sock_recv_file(eventLoop,sockfd,pClient,AE_READABLE);
	
	return ;
}

void do_sync_from_storage(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	logDebug("do_sync_from_storage",...);
	storage_client_t *pClient;

	pClient = clientData;

	storage_nio_read(pClient->file.file_name,24,finish_sync_recv_file_name,finish_storage_CLOSE_WAIT_stat,"do_sync_from_storage");
	
	return ;	
}
/*sync_recv_file end*/

/*sync_send_file*/
static void finish_send_sync_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	storage_client_t *pClient;

	pClient = clientData;
	logDebug("finish_send_sync_file %s",pClient->file.file_name);
	
	pClient->sync_node->binlog_read_count = pClient->sync_node->temp_binlog_read_count;

	/*need check again by time ticks*/
	refresh_storage_client_file(pClient);
	if(check_count_stamp(eventLoop,sockfd,pClient) > 0)
		aeCreateTimeEvent(eventLoop,pClient->sync_node->sync_pool->check_count_stamp_millisec,check_count_stamp,pClient,NULL);
	return ;
}

static void sync_send_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	logDebug("sync_send_file",...);
	storage_client_t *pClient;

	pClient = clientData;

	pClient->sockfd = sockfd;

	pClient->file.proc = finish_send_sync_file;

	pClient->file.start_offlen = string_to_int(pClient->file.file_name + 15,4);
	pClient->file.file_size = string_to_int(pClient->file.file_name + 19,4);

	/*get all_trunk_name*/	
	memcpy(pClient->sync_node->temp_file_name,pClient->file.file_name,8);
	memcpy(pClient->file.file_name,pClient->file.file_name + 8,5);
	pClient->file.file_name[5] = '\0';
	logDebug("finish_sync_file_read_name %d,%d,%s",pClient->file.file_size,pClient->file.start_offlen,pClient->file.file_name);

	new_nb_sock_send_file(eventLoop,sockfd,pClient,AE_WRITABLE);

	return ;
}

static void sync_send_file_name(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	logDebug("sync_send_file_name",...);
	sync_node_t *sync_node;
	storage_client_t *pClient;
	
	pClient = clientData;
	sync_node = pClient->sync_node;

	LOCK_IF_ERROR("sync_send_file_name",sync_node->sync_pool->binlog_read_count_lock);
	fseek(sync_node->sync_pool->binlog_fp,sync_node->binlog_read_count,SEEK_SET);
	/*if error check again*/
	if(fgets(pClient->file.file_name,25,sync_node->sync_pool->binlog_fp) == NULL)
	{
		UNLOCK_IF_ERROR("sync_send_file_name",sync_node->sync_pool->binlog_read_count_lock);
		aeCreateTimeEvent(eventLoop,sync_node->sync_pool->check_count_stamp_millisec,check_count_stamp,pClient,NULL);
		return ;
	}
	sync_node->temp_binlog_read_count = ftell(sync_node->sync_pool->binlog_fp);
	UNLOCK_IF_ERROR("sync_send_file_name",sync_node->sync_pool->binlog_read_count_lock);
	logDebug("sync_send_file_name,%s",pClient->file.file_name);

	pClient->data.total_size = 0;

	pClient->data.buff = pClient->file.file_name;
	pClient->data.need_size = 24;
	pClient->data.proc = sync_send_file;
	pClient->data.final_proc = start_check_count_stamp;
	
	if(aeCreateFileEvent(eventLoop, sync_node->sockfd, AE_WRITABLE,nb_sock_send_data,pClient) != AE_OK)
	{
		logError(	"file :"__FILE__",line :%d"\
			"sync_connect CreateFileEvent failed.",\
			__LINE__);
		clean_storage_client(pClient);
		aeCreateTimeEvent(eventLoop,sync_node->sync_pool->check_count_stamp_millisec,check_count_stamp,pClient,NULL);
	}
	return ;
}
/*sync_send_file end*/
static int check_count_stamp(struct aeEventLoop *eventLoop, long long id, void *clientData)
{
	logDebug("check_count_stamp",...);
	sync_node_t *sync_node = (sync_node_t *)(((storage_client_t *)clientData)->sync_node);

	/*count_stamp < MAX count_stamp,sync_start*/
	LOCK_IF_ERROR("check_count_stamp",g_storage_service.trunk_file_pool.real_file_name_lock);
	if(memcmp(sync_node->temp_file_name,sync_node->sync_pool->real_count_stamp_file_name,8) < 0)
	{
		UNLOCK_IF_ERROR("check_count_stamp",g_storage_service.trunk_file_pool.real_file_name_lock);
		sync_send_file_name(eventLoop,sync_node->sockfd,clientData,AE_NONE);
		return -1;
	}
	UNLOCK_IF_ERROR("check_count_stamp",g_storage_service.trunk_file_pool.real_file_name_lock);
	/* else again*/
	return sync_node->sync_pool->check_count_stamp_millisec;
}

static void start_check_count_stamp(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	logDebug("start_check_count_stamp",...);
	sync_node_t *sync_node = (sync_node_t *)(((storage_client_t *)clientData)->sync_node);

	aeCreateTimeEvent(eventLoop,sync_node->sync_pool->check_count_stamp_millisec,check_count_stamp,clientData,NULL);

	return ;
}

static void restart_sync_connect(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	logDebug("restart_sync_connect",...);
	sync_node_t *sync_node = (sync_node_t *)(((storage_client_t *)clientData)->sync_node);

	aeDeleteFileEvent(eventLoop,sockfd,mask);
	clean_storage_client(clientData);
	close(sockfd);
	aeCreateTimeEvent(eventLoop,sync_node->sync_pool->sync_connect_millisec,sync_connect,clientData,NULL);

	return;
}

int sync_connect(struct aeEventLoop *eventLoop, long long id, void *clientData)
{
	sync_node_t *sync_node = (sync_node_t *)clientData;
	storage_client_t *pClient;

	logDebug("connect to synced storage",...);
	printf("connect to synced storage\n");
	fflush(stdout);

	char inet_buf[INET_ADDRSTRLEN];
	if((sync_node->sockfd = connect_ip(inet_ntop(AF_INET,&sync_node->sync_address_info.ip,inet_buf,INET_ADDRSTRLEN),\
				sync_node->sync_address_info.port)) < 0)
	{
		close(sync_node->sockfd);
		return sync_node->sync_pool->sync_connect_millisec;
	}
	setnonblocking(sync_node->sockfd);
	
	if(sync_node->if_need_recovery == 0)
	{
		sync_node->temp_file_name[0] = '/';
		sync_node->sync_pool->sync_start_cmd = SYNC_START_FROM_STORAGE;

		/*sync_nio*/
		pClient = init_storage_client(&sync_node->nio_node);
	
		pClient->sync_node = sync_node;

		pClient->data.buff = &sync_node->sync_pool->sync_start_cmd;
		pClient->data.need_size = sizeof(sync_node->sync_pool->sync_start_cmd);
		pClient->data.proc = start_check_count_stamp;
		pClient->data.final_proc = restart_sync_connect;
		
		if(aeCreateFileEvent(eventLoop, sync_node->sockfd, AE_WRITABLE,nb_sock_send_data,pClient) != AE_OK)
		{
			logError(	"file :"__FILE__",line :%d"\
				"sync_connect CreateFileEvent failed.",\
				__LINE__);
			clean_storage_client(pClient);
			return sync_node->sync_pool->sync_connect_millisec;
		}
		/*end*/
	}
	else
	{
		sync_node->binlog_read_count = 0;
		sync_node->synclog_read_count = 0;

		sync_node->temp_file_name[0] = '/';
		sync_node->sync_pool->sync_start_cmd = RECOVERY_START_FROM_STORAGE;

		/*sync_nio*/
		pClient = init_storage_client(&sync_node->nio_node);
	
		pClient->sync_node = sync_node;

		pClient->data.buff = &sync_node->sync_pool->sync_start_cmd;
		pClient->data.need_size = sizeof(sync_node->sync_pool->sync_start_cmd);
		pClient->data.proc = start_check_recovery_count_stamp;
		pClient->data.final_proc = restart_sync_connect;
		
		if(aeCreateFileEvent(eventLoop, sync_node->sockfd, AE_WRITABLE,nb_sock_send_data,pClient) != AE_OK)
		{
			logError(	"file :"__FILE__",line :%d"\
				"sync_connect CreateFileEvent failed.",\
				__LINE__);
			clean_storage_client(pClient);
			return sync_node->sync_pool->sync_connect_millisec;
		}
	}
	return -1;
}

static aeEventLoop* create_sync_worker(aeEventLoop *eventLoop,sync_node_t *sync_node)
{
	logDebug("create_sync_worker",...);
	if((eventLoop = (aeEventLoop*)aeCreateEventLoop()) == NULL)
	{
		logError(	"file :"__FILE__",line :%d"\
			"create_tracker_service CreateEventLoop failed.");
			return NULL;
	}

	sync_node->nio_node.eventLoop = eventLoop;
	
	if(init_stop_sync_pipe(sync_node) < 0)
		return NULL;

	if(aeCreateFileEvent(eventLoop, sync_node->stop_fd[0], AE_READABLE,sync_stop,NULL) != AE_OK)
	{
		logError(	"file :"__FILE__",line :%d"\
			"create_sync_worker CreateFileEvent failed.",\
			__LINE__);
		return NULL;
	}

	init_aio_and_nio_node(&sync_node->nio_node);

	aeCreateTimeEvent(eventLoop,sync_node->sync_pool->sync_connect_millisec,sync_connect,sync_node,NULL);
	
	return eventLoop;
}

static void run_sync_worker(aeEventLoop *eventLoop)
{
	logDebug("run_sync_worker",...);
	aeSetBeforeSleepProc(eventLoop,NULL);
	aeMain(eventLoop);
	aeDeleteEventLoop(eventLoop);

	return;
}

static void* new_sync_thread_entrance(void *arg)
{
	logDebug("new_sync_thread_entrance",...);
	aeEventLoop *eventLoop = NULL;

	if((eventLoop = create_sync_worker(eventLoop,arg)) == NULL)
		return NULL;
	
	run_sync_worker(eventLoop);
	return NULL;	
}

int init_new_sync_thread(sync_pool_t *sync_pool,int storage_id,storage_address_info_t *storage_address_info,char if_need_recovery)
{
	logDebug("init_new_sync_thread",...);
	memcpy(&sync_pool->sync_node[storage_id].sync_address_info,storage_address_info,sizeof(*(storage_address_info)));

	sync_pool->sync_node[storage_id].if_need_recovery = if_need_recovery;
	
	if(sync_pool->sync_node[storage_id].node_pid != 0)
		pthread_join(sync_pool->sync_node[storage_id].node_pid,NULL);

	pthread_create(&sync_pool->sync_node[storage_id].node_pid,NULL,new_sync_thread_entrance,sync_pool->sync_node+storage_id);

	return 0;
}

