#include <arpa/inet.h>
#include "storage_net.h"
#include "storage_service.h"
#include "tracker_proto.h"
#include "storage_proto.h"
#include "sockopt.h"
#include "storage_global.h"
#include "storage_sync.h"
#include "storage_recovery.h"
#include "base.h"
#include "share_fun.h"
#include "heart_beat.h"

/*heart_beat*/
static void finish_recv_return_heart_beat_head(struct aeEventLoop *eventLoop,int sockfd,void *clientData,int mask);
static void start_recv_return_heart_beat_head(struct aeEventLoop *eventLoop,int sockfd,void *clientData,int mask);
static int heart_beat(struct aeEventLoop *eventLoop, long long id, void *clientData);
static void start_send_heart_beat(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
static void restart_heart_beat(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
/*heart_beat_end*/

/*upload_to_storage*/
static void recv_files_upload_to_storage(struct aeEventLoop *eventLoop, int incomesock, void *clientData, int mask);
/*upload_to_storage_end*/

static void finish_recv_return_heart_beat_head(struct aeEventLoop *eventLoop,int sockfd,void *clientData,int mask)
{
	storage_client_t *pClient;
	heart_beat_t *hb;
	return_heart_beat_t *r_hb;
	int status;
	printf("finish_recv_return_heart_beat_head\n");
	fflush(stdout);
		
	pClient = (storage_client_t *)clientData;
	hb = &g_storage_service.heart_beat;
	r_hb = &g_storage_service.return_heart_beat;
	
	printf("my status is %d,will be %d,my group_id is %d,storage_id is %d\n",\
			hb->status,\
			r_hb->head.status,\
			hb->group_id,\
			r_hb->head.storage_id);
	fflush(stdout);

	status = hb->status;	
	LOCK_IF_ERROR("finish_recv_return_heart_beat_head",g_storage_service.heart_beat_lock);
	hb->status = r_hb->head.status;
	hb->storage_id = r_hb->head.storage_id;
	UNLOCK_IF_ERROR("finish_recv_return_heart_beat_head",g_storage_service.heart_beat_lock);

	if(status == STORAGE_OFFLINE)
	{
		if(r_hb->head.status == STORAGE_RECOVERY)
		{
			pClient->data.buff = &g_storage_service.sync_pool.send_count_stamp;
			pClient->data.need_size = sizeof(count_stamp_t);
			pClient->data.total_size = 0;
			pClient->data.proc = start_send_heart_beat;
	 
			if(aeCreateFileEvent(eventLoop, g_storage_service.tracker_info.sockfd, AE_READABLE,nb_sock_recv_data,pClient) != AE_OK)
			{
				logError(	"file :"__FILE__",line :%d"\
					"start_recv_storage_list_info CreateFileEvent failed.",\
					__LINE__);
				clean_storage_client(pClient);
				close(g_storage_service.tracker_info.sockfd);
				aeCreateTimeEvent(eventLoop,g_storage_service.heart_beat_millisec,heart_beat_init,NULL,NULL);
				return ;
			}	
			return ;
		}
		start_send_heart_beat(eventLoop,sockfd,pClient,AE_NONE);
		return ;
	}
	if(status == STORAGE_RECOVERY)
	{
		start_send_heart_beat(eventLoop,sockfd,pClient,AE_NONE);
		return ;
	}
	if(status == STORAGE_ACTIVE)
	{
		start_send_heart_beat(eventLoop,sockfd,pClient,AE_NONE);
		return ;
	}
	/*storage must be STORAGE_ONLINE or STORAGE_RECOVERYED*/
	
	if(status == STORAGE_RECOVERYED)
	{
		start_send_heart_beat(eventLoop,sockfd,pClient,AE_NONE);
		return ;
	}
	
	/*now must be STORAGE_ONLINE*/
	if(r_hb->head.unknow_new_num == 0 && \
		r_hb->head.unknow_dea_num == 0)
	{
		start_send_heart_beat(eventLoop,sockfd,pClient,AE_NONE);
		return ;
	}
	
	/*malloc space for new online storage*/
	
	/*tracker_nio*/
	r_hb->body.new_storage = \
		malloc((r_hb->head.unknow_new_num + \
			r_hb->head.unknow_dea_num) *
			sizeof(*r_hb->body.new_storage));

	pClient->data.buff = r_hb->body.new_storage;
	pClient->data.need_size = (r_hb->head.unknow_new_num + \
				r_hb->head.unknow_dea_num) * \
				sizeof(*r_hb->body.new_storage);
	pClient->data.total_size = 0;
	pClient->data.proc = start_send_heart_beat;
	 
	if(aeCreateFileEvent(eventLoop, g_storage_service.tracker_info.sockfd, AE_READABLE,nb_sock_recv_data,pClient) != AE_OK)
	{
		logError(	"file :"__FILE__",line :%d"\
				"start_recv_storage_list_info CreateFileEvent failed.",\
				__LINE__);
		clean_storage_client(pClient);
		close(g_storage_service.tracker_info.sockfd);
		aeCreateTimeEvent(eventLoop,g_storage_service.heart_beat_millisec,heart_beat_init,NULL,NULL);
		return ;
	}
	/*end*/
	return ;
}

static void start_recv_return_heart_beat_head(struct aeEventLoop *eventLoop,int sockfd,void *clientData,int mask)
{
	storage_client_t *pClient;
	printf("start_recv_return_heart_beat_head\n");
	fflush(stdout);
	
	pClient = (storage_client_t *)clientData;
	
	/*tracker_nio*/
	pClient->data.buff = &g_storage_service.return_heart_beat.head;
	pClient->data.need_size = sizeof(g_storage_service.return_heart_beat.head);
	pClient->data.total_size = 0;
	pClient->data.proc = finish_recv_return_heart_beat_head;
	 
	if(aeCreateFileEvent(eventLoop, g_storage_service.tracker_info.sockfd, AE_READABLE,nb_sock_recv_data,pClient) != AE_OK)
	{
		logError(	"file :"__FILE__",line :%d"\
			"start_recv_return_heart_beat_head CreateFileEvent failed.",\
			__LINE__);
		clean_storage_client(pClient);
		close(g_storage_service.tracker_info.sockfd);
		aeCreateTimeEvent(eventLoop,g_storage_service.heart_beat_millisec,heart_beat_init,NULL,NULL);
		return ;
	}
	/*end*/
	return ;
}

static int heart_beat(struct aeEventLoop *eventLoop, long long id, void *clientData)
{
	storage_client_t *pClient;
#ifdef __DEBUG__
		logDebug("send heartbeat...",...);
		printf("send heartbeat...\n");
		fflush(stdout);
#endif
	pClient = (storage_client_t *)clientData;
	
	/*tracker_nio*/
	pClient->data.buff = &g_storage_service.heart_beat;
	pClient->data.need_size = sizeof(g_storage_service.heart_beat);
	pClient->data.total_size = 0;
	pClient->data.proc = start_recv_return_heart_beat_head;
	 
	if(aeCreateFileEvent(eventLoop, g_storage_service.tracker_info.sockfd, AE_WRITABLE,nb_sock_send_data,pClient) != AE_OK)
	{
		logError(	"file :"__FILE__",line :%d"\
			"heart_beat CreateFileEvent failed.",\
			__LINE__);
		clean_storage_client(pClient);
		close(g_storage_service.tracker_info.sockfd);
		aeCreateTimeEvent(eventLoop,g_storage_service.heart_beat_millisec,heart_beat_init,NULL,NULL);
		return -1;
	}
	/*end*/
	return -1;
}

static void start_send_heart_beat(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{	
	printf("start_send_heart_beat_info\n");
	fflush(stdout);

	int i;
	char dummy;
	return_heart_beat_t *r_hb;

	i = 0;
	r_hb = &g_storage_service.return_heart_beat;

	for(;i != r_hb->head.unknow_new_num;++i)
	{
		g_storage_service.heart_beat.know_sf[(int)r_hb->body.new_storage[i].storage_id] = 1;
		printf("new_online_storage_id %d\n",r_hb->body.new_storage[i].storage_id);
		fflush(stdout);
		init_new_sync_thread(&g_storage_service.sync_pool,r_hb->body.new_storage[i].storage_id,
			&r_hb->body.new_storage[i].storage_address_info,r_hb->body.new_storage[i].if_need_recovery);
	}
	for(;i != r_hb->head.unknow_new_num + \
			r_hb->head.unknow_dea_num;++i)
	{
		g_storage_service.heart_beat.know_sf[(int)r_hb->body.new_storage[i].storage_id] = 0;
		if(write(g_storage_service.sync_pool.sync_node[(int)r_hb->body.new_storage[i].storage_id].stop_fd[1],&dummy,1) == -1)
		{
			/*do nothing,just to avoid the warning*/
		}
		printf("new_dead_storage_id %d\n",r_hb->body.new_storage[i].storage_id);
		fflush(stdout);
	}
	if(r_hb->head.unknow_new_num != 0 || \
		r_hb->head.unknow_dea_num != 0)
		free(r_hb->body.new_storage);
	aeCreateTimeEvent(eventLoop,g_storage_service.heart_beat_millisec,heart_beat,clientData,NULL);
	return ;
}

static void restart_heart_beat(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	storage_client_t *pClient;

	pClient = (storage_client_t*)clientData;
	
	aeDeleteFileEvent(eventLoop,sockfd,mask);
	close(sockfd);
	aeCreateTimeEvent(eventLoop,g_storage_service.heart_beat_millisec,heart_beat_init,pClient->nio_node,NULL);
	clean_storage_client(pClient);

	return;
}

int heart_beat_init(struct aeEventLoop *eventLoop, long long id, void *clientData)
{
	printf("heart_beat_init\n");
	fflush(stdout);
	storage_client_t *pClient;
	
	if((g_storage_service.tracker_info.sockfd = connect_ip(g_storage_service.tracker_info.ip,g_storage_service.tracker_info.port)) < 0)
	{
		close(g_storage_service.tracker_info.sockfd);
		return g_storage_service.heart_beat_millisec;
	}
	setnonblocking(g_storage_service.tracker_info.sockfd);
	
	g_storage_service.heart_beat_cmd = STORAGE_HEART_BEAT;
		
	/*tracker_nio*/
	pClient = init_storage_client(clientData);
	pClient->data.buff = &g_storage_service.heart_beat_cmd;
	pClient->data.need_size = sizeof(g_storage_service.heart_beat_cmd);
	pClient->data.proc = start_send_heart_beat;
	pClient->data.final_proc = restart_heart_beat;
	
	if(aeCreateFileEvent(eventLoop, g_storage_service.tracker_info.sockfd, AE_WRITABLE,nb_sock_send_data,pClient) != AE_OK)
	{
		logError(	"file :"__FILE__",line :%d"\
			"heart_beat_init CreateFileEvent failed.",\
			__LINE__);
		clean_storage_client(pClient);
		return g_storage_service.heart_beat_millisec;
	}
	/*end*/
	return -1;
}

/*upload_file*/
static void finish_file_send_name(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	storage_client_t *pClient;

	pClient = (storage_client_t *)clientData;

	if(aeCreateFileEvent(eventLoop, sockfd, AE_READABLE,finish_storage_CLOSE_WAIT_stat,pClient) != AE_OK)
	{
		logError(	"file :"__FILE__",line :%d"\
			"recv_files_upload_to_storage CreateFileEvent failed.\n",\
			__LINE__);
		clean_storage_client(pClient);
		return ;
	}
	return ;
}

static void recv_files_upload_to_storage(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	storage_client_t *pClient;

	pClient = (storage_client_t *)clientData;
	
	LOCK_IF_ERROR("recv_files_upload_to_storage",g_storage_service.heart_beat_lock);

	g_storage_service.heart_beat.storage_size += pClient->file.read_size;
	g_storage_service.heart_beat.upload_size += pClient->file.read_size;
	int_to_string(g_storage_service.heart_beat.group_id,pClient->file.file_name+13,2);

	UNLOCK_IF_ERROR("recv_files_upload_to_storage",g_storage_service.heart_beat_lock);

	pthread_mutex_lock(&g_storage_service.trunk_file_pool.real_file_name_lock);

	memcpy(pClient->file.file_name,get_real_file_name(&g_storage_service.trunk_file_pool),8);

	pthread_mutex_unlock(&g_storage_service.trunk_file_pool.real_file_name_lock);

	int_to_string(pClient->file.read_size,pClient->file.file_name+19,4);
	int_to_string(pClient->file.start_offlen,pClient->file.file_name+15,4);
	
	logDebug("%s uploaded,%d,%d",pClient->file.file_name,pClient->file.read_size,pClient->file.start_offlen);

	pClient->file.file_name[23] = '\n';

	LOCK_IF_ERROR("recv_files_upload_to_storage",g_storage_service.sync_pool.binlog_read_count_lock);
	write_file_log(pClient->file.file_name,24,g_storage_service.sync_pool.binlog_fp);
	UNLOCK_IF_ERROR("recv_files_upload_to_storage",g_storage_service.sync_pool.binlog_read_count_lock);

	storage_nio_write(pClient->file.file_name,24,finish_file_send_name,NULL,"recv_files_upload_to_storage");
	
	return ;
}

void do_upload_to_storage(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	storage_client_t *pClient;
	
	pClient = clientData;
	
	pClient->file.proc = recv_files_upload_to_storage;
	if(pClient->file.need_name == 1)
		pClient->file.need_name = 0;
	else
		pClient->file.need_name = 1;
	
	nb_sock_recv_file(eventLoop,sockfd,pClient,AE_READABLE);

	return ;
}
/*upload file end*/

/*download file*/
static void finish_download_from_storage(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	if(aeCreateFileEvent(eventLoop, sockfd, AE_READABLE,finish_storage_CLOSE_WAIT_stat,clientData) != AE_OK)
	{
		logError(	"file :"__FILE__",line :%d"\
			"accept_command CreateFileEvent failed.\n",\
			__LINE__);
		clean_storage_client((storage_client_t*)clientData);
		return ;
	}
	return ;
}


static void finish_file_recv_name(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	storage_client_t *pClient;

	pClient = clientData;
	
	pClient->file.proc = finish_download_from_storage;

	pClient->file.start_offlen = string_to_int(pClient->file.file_name + 15,4);
	pClient->file.file_size = string_to_int(pClient->file.file_name + 19,4);

	logDebug("finish_download_recv_name %s",pClient->file.file_name);
	/*get all_trunk_name*/	
	memcpy(pClient->file.file_name,pClient->file.file_name+8,5);
	pClient->file.file_name[5] = '\0';
	logDebug("finish_download_recv_trunk_name %d,%d,%s",pClient->file.file_size,pClient->file.start_offlen,pClient->file.file_name);

	new_nb_sock_send_file(eventLoop,sockfd,pClient,AE_WRITABLE);
	return ;
}

static void do_download_from_storage(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	storage_client_t *pClient;

	pClient = clientData;

	storage_nio_read(pClient->file.file_name,24,finish_file_recv_name,NULL,"do_download_from_storage");
	
	return ;
}
/*download file end*/

void accept_command(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	storage_client_t *pClient;

	pClient = (storage_client_t *)clientData;
	
	/*see do in this file*/
	if(pClient->cmd == UPLOAD_TO_STORAGE)
	{
		do_upload_to_storage(eventLoop,sockfd,pClient,AE_READABLE);
		return ;
	}
	/*see do in this file*/
	if(pClient->cmd == DOWNLOAD_FROM_STORAGE)
	{
		do_download_from_storage(eventLoop,sockfd,pClient,AE_READABLE);
		return ;
	}
	/*see do in storage_sync.c*/
	if(pClient->cmd == SYNC_START_FROM_STORAGE)
	{
		do_sync_from_storage(eventLoop,sockfd,pClient,AE_READABLE);
		return ;
	}
	/*see do in storage_recovery.c*/
	if(pClient->cmd == RECOVERY_START_FROM_STORAGE)
	{
		do_recovery_from_storage(eventLoop,sockfd,pClient,AE_READABLE);
		return ;
	}
	return ;
}
