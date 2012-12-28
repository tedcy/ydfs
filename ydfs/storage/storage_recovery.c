#include "storage_recovery.h"
#include "storage_global.h"
#include "base.h"
#include "fcntl.h"
#include "storage_net.h"
/*
 *get the binlog_read_count by send_count_stamp
 */
static void get_binlog_read_count(char *recovery_count_stamp,int len);

static void finish_storage_recovery(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
/*recovery_recv_file*/
/*
**we first recv the file name,then call nb_recv_file
*/
static void finish_recv_recovery_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
static void finish_recovery_recv_file_name(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
/*recovery_recv_file end*/

static int check_recovery_count_stamp(struct aeEventLoop *eventLoop, long long id, void *clientData);

static void get_binlog_read_count(char *recovery_count_stamp,int len)
{
	int storage_iter;

	for(storage_iter = 0;storage_iter != STORAGE_MAXSIZE;++storage_iter)
	{
		if(memcmp(g_storage_service.sync_pool.send_count_stamp.storage[storage_iter],recovery_count_stamp,8) == 0)
			g_storage_service.sync_pool.sync_node[storage_iter].binlog_read_count = ftell(g_storage_service.sync_pool.binlog_fp);
	}	
	return ;
}

static void finish_storage_recovery(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	g_storage_service.heart_beat.status = STORAGE_RECOVERYED;
	aeDeleteFileEvent(eventLoop,sockfd,AE_READABLE);
	clean_storage_client((storage_client_t*)clientData);
	return ;
}

/*recovery_recv_file*/
static void finish_recv_recovery_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	storage_client_t *pClient;
	pClient = clientData;

	logDebug("finish_recv_recovery_file %s",pClient->file.file_name);

	LOCK_IF_ERROR("finish_recv_recovery_file",g_storage_service.heart_beat_lock);
	g_storage_service.heart_beat.storage_size += pClient->file.read_size;
	g_storage_service.heart_beat.upload_size += pClient->file.read_size;
	UNLOCK_IF_ERROR("finish_recv_recovery_file",g_storage_service.heart_beat_lock);

	memcpy(g_storage_service.sync_pool.real_count_stamp_file_name,pClient->file.real_file_name,8);

	LOCK_IF_ERROR("finish_recv_recovery_file",g_storage_service.sync_pool.binlog_read_count_lock);
	write_file_log(pClient->file.real_file_name,24,g_storage_service.sync_pool.binlog_fp);
	UNLOCK_IF_ERROR("finish_recv_recovery_file",g_storage_service.sync_pool.binlog_read_count_lock);

	get_binlog_read_count(pClient->file.real_file_name,8);

	refresh_storage_client_file(clientData);
	do_recovery_from_storage(eventLoop,sockfd,clientData,AE_NONE);
	return ;	
}

static void finish_recovery_recv_file_name(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	storage_client_t *pClient;
	pClient = clientData;
	
	pClient->file.proc = finish_recv_recovery_file;

	pClient->file.start_offlen = string_to_int(pClient->file.file_name + 15,4);
	pClient->file.file_size = string_to_int(pClient->file.file_name + 19,4);

	memcpy(pClient->file.real_file_name,pClient->file.file_name,24);

	logDebug("finish_recovery_recv_file_name %s",pClient->file.file_name);
	/*get all_trunk_name*/	
	memcpy(pClient->file.file_name,pClient->file.file_name+8,5);
	pClient->file.file_name[5] = '\0';
	logDebug("finish_recovery_recv_trunk_name %d,%d,%s",pClient->file.file_size,pClient->file.start_offlen,pClient->file.file_name);

	if((pClient->file.fd = open(pClient->file.file_name,O_WRONLY|O_CREAT,0664)) == -1)
	{
		logError(	"file: "__FILE__",line :%d, "\
			"finish_recovery_recv_file_name open file failed,"\
			"errno: %d,error info: %s",\
			__LINE__,errno,strerror(errno));
		*(char *)NULL = 0;
	//	exit(1);
	}
	lseek(pClient->file.fd,pClient->file.start_offlen,SEEK_SET);

	nb_sock_recv_file(eventLoop,sockfd,pClient,AE_READABLE);
	
	return ;
}

void do_recovery_from_storage(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	logDebug("do_recovery_from_storage",...);
	storage_client_t *pClient;

	pClient = clientData;

	storage_nio_read(pClient->file.file_name,24,finish_recovery_recv_file_name,finish_storage_recovery,"do_recovery_from_storage");
	
	return ;	
}
/*recovery_recv_file end*/

/*recovery_send_file*/
static void finish_send_recovery_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	storage_client_t *pClient;

	pClient = clientData;
	logDebug("finish_send_recovery_file %s",pClient->file.file_name);
	
	pClient->sync_node->synclog_read_count = pClient->sync_node->temp_synclog_read_count;

	/*need check again by time ticks*/
	refresh_storage_client_file(pClient);
	if(check_recovery_count_stamp(eventLoop,sockfd,pClient) > 0)
	{
		pClient->sync_node->if_need_recovery = 0;
		close(sockfd);
		aeCreateTimeEvent(eventLoop,pClient->sync_node->sync_pool->sync_connect_millisec,sync_connect,pClient->sync_node,NULL);
		return ;
	}
	return ;
}

static void recovery_send_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	logDebug("recovery_send_file",...);
	storage_client_t *pClient;

	pClient = clientData;

	pClient->sockfd = sockfd;

	pClient->file.proc = finish_send_recovery_file;

	pClient->file.start_offlen = string_to_int(pClient->file.file_name + 15,4);
	pClient->file.file_size = string_to_int(pClient->file.file_name + 19,4);

	/*get all_trunk_name*/	
	memcpy(pClient->sync_node->temp_file_name,pClient->file.file_name,8);
	memcpy(pClient->file.file_name,pClient->file.file_name + 8,5);
	pClient->file.file_name[5] = '\0';
	logDebug("finish_recovery_file_read_name %d,%d,%s",pClient->file.file_size,pClient->file.start_offlen,pClient->file.file_name);

	new_nb_sock_send_file(eventLoop,sockfd,pClient,AE_WRITABLE);

	return ;
}

static void recovery_send_file_name(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	logDebug("recovery_send_file_name",...);
	sync_node_t *sync_node;
	storage_client_t *pClient;
	
	pClient = clientData;
	sync_node = pClient->sync_node;

	LOCK_IF_ERROR("recovery_send_file_name",sync_node->sync_pool->synclog_read_count_lock);
	fseek(sync_node->sync_pool->synclog_fp,sync_node->synclog_read_count,SEEK_SET);
	/*if error check again*/
	while(1)
	{
		if(fgets(pClient->file.file_name,25,sync_node->sync_pool->synclog_fp) == NULL)
		{
			UNLOCK_IF_ERROR("recovery_send_file_name",sync_node->sync_pool->synclog_read_count_lock);
			sync_node->if_need_recovery = 0;
			close(sockfd);
			aeCreateTimeEvent(eventLoop,pClient->sync_node->sync_pool->sync_connect_millisec,sync_connect,pClient->sync_node,NULL);
			return ;
		}
		if(string_to_int(pClient->file.file_name + 12,1) != sync_node->storage_id)
			continue ;
		break ;
	}
	sync_node->temp_synclog_read_count = ftell(sync_node->sync_pool->synclog_fp);
	UNLOCK_IF_ERROR("recovery_send_file_name",sync_node->sync_pool->synclog_read_count_lock);
	logDebug("recovery_send_file_name,%s",pClient->file.file_name);

	pClient->data.total_size = 0;

	pClient->data.buff = pClient->file.file_name;
	pClient->data.need_size = 24;
	pClient->data.proc = recovery_send_file;
	pClient->data.final_proc = start_check_recovery_count_stamp;
	
	if(aeCreateFileEvent(eventLoop, sync_node->sockfd, AE_WRITABLE,nb_sock_send_data,pClient) != AE_OK)
	{
		logError(	"file :"__FILE__",line :%d"\
			"recovery_connect CreateFileEvent failed.",\
			__LINE__);
		clean_storage_client(pClient);
		aeCreateTimeEvent(eventLoop,sync_node->sync_pool->check_recovery_count_stamp_millisec,check_recovery_count_stamp,pClient,NULL);
	}
	return ;
}
/*recovery_send_file end*/
static int check_recovery_count_stamp(struct aeEventLoop *eventLoop, long long id, void *clientData)
{
	logDebug("check_recovery_count_stamp",...);
	sync_node_t *sync_node = (sync_node_t *)(((storage_client_t *)clientData)->sync_node);

	/*success_count_stamp < sync count_stamp,recovery_start*/
	if(memcmp(sync_node->temp_file_name,sync_node->sync_pool->count_stamp->storage[sync_node->storage_id],8) < 0)
	{
		recovery_send_file_name(eventLoop,sync_node->sockfd,clientData,AE_NONE);
		return -1;
	}
	/* else again*/
	return sync_node->sync_pool->check_recovery_count_stamp_millisec;
}

void start_check_recovery_count_stamp(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	logDebug("start_check_recovery_count_stamp",...);
	sync_node_t *sync_node = (sync_node_t *)(((storage_client_t *)clientData)->sync_node);

	aeCreateTimeEvent(eventLoop,sync_node->sync_pool->check_recovery_count_stamp_millisec,check_recovery_count_stamp,clientData,NULL);

	return ;
}
