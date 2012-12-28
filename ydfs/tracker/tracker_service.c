#include "tracker_service.h"
#include "tracker_proto.h"
#include "tracker_global.h"
#include "tracker_net.h"
#include "storage_pool.h"
#include "base.h"
#include "heart_beat.h"
#include "share_fun.h"

/*download_get_storage*/
static void finish_send_best_upload_storage_data(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
static void do_upload_get_storage(struct aeEventLoop *eventLoop, int incomesock, void *clientData, int mask);
/*download_get_storage end*/

/*download_get_storage*/
static void finish_send_best_download_storage_address(struct aeEventLoop *eventLoop, int incomesock, void *clientData, int mask);
static void start_send_best_download_storage_address(struct aeEventLoop *eventLoop, int incomesock, void *clientData, int mask);
static void do_download_get_storage(struct aeEventLoop *eventLoop, int incomesock, void *clientData, int mask);
/*download_get_storage end*/

static void finish_send_return_heart_beat_head(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
static void finish_recv_heart_beat(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
static void do_storage_heart_beat(struct aeEventLoop *eventLoop, int incomesock, void *clientData, int mask);

/*do_upload_get_storage*/
static void finish_send_best_upload_storage_data(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	if(aeCreateFileEvent(eventLoop, sockfd, AE_READABLE,finish_tracker_CLOSE_WAIT_stat,clientData) != AE_OK)
	{
		logError(	"file :"__FILE__",line :%d"\
			"finish_send_best_storage_data CreateFileEvent failed.\n",\
			__LINE__);
		clean_tracker_client((tracker_client_t*)clientData);
		return ;
	}
	return ;
}

static void do_upload_get_storage(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	tracker_client_t *pClient;
	
	pClient = clientData;
	
	get_best_storage(&pClient->storage);

	tracker_nio_write(&pClient->storage,sizeof(pClient->storage),\
			finish_send_best_upload_storage_data,NULL,"do_upload_get_storage");
	
	return ;
}
/*do_upload_get_storage*/

/*do_download_get_storage*/
static void finish_send_best_download_storage_address(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	if(aeCreateFileEvent(eventLoop, sockfd, AE_READABLE,finish_tracker_CLOSE_WAIT_stat,clientData) != AE_OK)
	{
		logError(	"file :"__FILE__",line :%d"\
			"finish_send_best_storage_data CreateFileEvent failed.\n",\
			__LINE__);
		clean_tracker_client((tracker_client_t*)clientData);
		return ;
	}
	return ;
}

static void start_send_best_download_storage_address(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	tracker_client_t *pClient;
	
	pClient = clientData;

	find_download_storage(pClient->download_filename,&pClient->storage);
	
	tracker_nio_write(&pClient->storage,sizeof(pClient->storage),\
			finish_send_best_download_storage_address,NULL,"start_send_best_download_storage_address");

	return ;
}

static void do_download_get_storage(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	tracker_client_t *pClient;

	pClient = clientData;
	
	tracker_nio_read(pClient->download_filename,24,\
			start_send_best_download_storage_address,NULL,"do_download_get_storage");
	
	return ;
}
/*do_download_get_storage end*/

void heart_beat_stop(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	tracker_client_t *pClient;

	pClient = (tracker_client_t *)clientData;
	
	if(pClient->heart_beat != NULL)
	{
		dead_heart_beat(pClient->heart_beat->group_id,pClient->heart_beat->storage_id);
		aeDeleteFileEvent(eventLoop,sockfd,mask);
		clean_tracker_client(clientData);
	}
	return ;
}

static void finish_send_return_heart_beat_head(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	tracker_client_t *pClient;
	int return_heart_beat_storage_index;
	heart_beat_t *hb;
	return_heart_beat_t *r_hb;
	group_node_t *group;
	storage_node_t *storage;
	printf("finish_send_return_heart_beat_head\n");
	fflush(stdout);

	pClient = (tracker_client_t *)clientData;
	hb = pClient->heart_beat;
	r_hb = pClient->return_heart_beat;
	group = g_tracker_service.storage_pool.group + (int)hb->group_id;
	storage = group->storage + (int)hb->storage_id;

	if(hb->status == STORAGE_OFFLINE)
	{
		if(r_hb->head.status == STORAGE_RECOVERY)
		{
			tracker_nio_write(&storage->send_count_stamps,\
				sizeof(count_stamp_t),\
			do_storage_heart_beat,heart_beat_stop,"finish_send_return_head_beat_head");
			return ;
		}
		do_storage_heart_beat(eventLoop,sockfd,pClient,AE_NONE);	
		return ;
	}
	if(hb->status == STORAGE_RECOVERY)
	{
		do_storage_heart_beat(eventLoop,sockfd,pClient,AE_NONE);	
		return ;
	}
	if(hb->status == STORAGE_ACTIVE)
	{
		do_storage_heart_beat(eventLoop,sockfd,pClient,AE_NONE);	
		return ;
	}
	/*now storage must be STORAGE_ONLINE or STORAGE_RECOVERYED*/
	if(r_hb->head.unknow_new_num != 0||r_hb->head.unknow_dea_num != 0)
	{
		for(return_heart_beat_storage_index = 0;\
			return_heart_beat_storage_index != r_hb->head.unknow_new_num + r_hb->head.unknow_dea_num;\
				++return_heart_beat_storage_index)
		{
			LOCK_IF_ERROR("finish_send_return_heart_beat_head",storage->lock);
			storage->know_sf[(int)r_hb->body.new_storage[return_heart_beat_storage_index].storage_id] = \
			group->real_sf[(int)r_hb->body.new_storage[return_heart_beat_storage_index].storage_id];
			UNLOCK_IF_ERROR("finish_send_return_heart_beat_head",storage->lock);
		}	
		/*read the sock to wait another heart_beat*/
		tracker_nio_write(r_hb->body.new_storage,\
			(r_hb->head.unknow_new_num + r_hb->head.unknow_dea_num)*\
				sizeof(*(r_hb->body.new_storage)),\
			do_storage_heart_beat,heart_beat_stop,"finish_send_return_head_beat_head");
		return ;
	}
	
	do_storage_heart_beat(eventLoop,sockfd,pClient,AE_NONE);	
	return ;
}

static void finish_recv_heart_beat(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	tracker_client_t *pClient;
	in_addr_t ip;
	printf("finish_recv_heart_beat\n");
	fflush(stdout);
	
	pClient = (tracker_client_t *)clientData;
		
	if(get_peer_ip(sockfd,&ip) < 0)
	{
		clean_tracker_client(pClient);
		return ;
	}
	
	insert_heart_beat(pClient->heart_beat,ip,pClient->return_heart_beat);

	tracker_nio_write(&pClient->return_heart_beat->head,sizeof(pClient->return_heart_beat->head),\
			finish_send_return_heart_beat_head,heart_beat_stop,"finish_recv_heart_beat");
	
	return ;
}

static void do_storage_heart_beat(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	tracker_client_t *pClient;

	pClient = (tracker_client_t *)clientData;
	printf("do_storage_heart_beat\n");
	fflush(stdout);
	
	pClient->data.buff = pClient->heart_beat;
	pClient->data.need_size = sizeof(*pClient->heart_beat);
	pClient->data.total_size = 0;
	pClient->data.proc = finish_recv_heart_beat;
	pClient->data.final_proc = heart_beat_stop;
	
	if(aeCreateFileEvent(eventLoop, sockfd, AE_READABLE,nb_sock_recv_data,pClient) != AE_OK)
	{
		logError(	"file :"__FILE__",line :%d"
			"do_storage_heart_beat"" CreateFileEvent failed.",
			__LINE__);
		exit(1) ;
	}
	return ;
}

int check_storage_pool(struct aeEventLoop *eventLoop,long long id,void *clientData)
{
	manage_storage_pool();
	return g_tracker_service.check_storage_pool_millisec;
}

void accept_command(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	tracker_client_t *pClient;

	pClient = (tracker_client_t *)clientData;
	
	if(pClient->cmd == UPLOAD_GET_STORAGE)
	{
		do_upload_get_storage(eventLoop,sockfd,pClient,AE_WRITABLE);
		return;
	}
	if(pClient->cmd == DOWNLOAD_GET_STORAGE)
	{
		do_download_get_storage(eventLoop,sockfd,pClient,AE_READABLE);
		return;
	}
	if(pClient->cmd == STORAGE_HEART_BEAT)
	{
		pClient->heart_beat = (heart_beat_t*)malloc(sizeof(*pClient->heart_beat));
		pClient->return_heart_beat = (return_heart_beat_t*)malloc(sizeof(*pClient->return_heart_beat));
	
		memset(pClient->heart_beat,0,sizeof(*pClient->heart_beat));
		memset(pClient->return_heart_beat,0,sizeof(*pClient->return_heart_beat));
	
		do_storage_heart_beat(eventLoop,sockfd,pClient,AE_READABLE);
		return;
	}
	return ;
}
