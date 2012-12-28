#ifndef STORAGE_GLOBAL_H
#define STORAGE_GLOBAL_H

#define __DEBUG__
#define __STORAGE__

#include <pthread.h>

#include "sockopt.h"
#include "storage_nio.h"
#include "aio.h"
#include "trunk_file.h"
#include "storage_sync.h"
#include "share_net.h"

/*connect to tracker*/
typedef struct TRACKER_INFO_T
{
	char ip[32];
	unsigned short port;
	int sockfd;	
}tracker_info_t;

typedef struct STORAGE_CLIENT_T
{
	nb_data_t data;
	char cmd;	
	int sockfd;
	nb_file_t file;
	aio_work_node_t *cur_aio_node;
	sync_node_t *sync_node;
	trunk_file_node_t *trunk_file_node;
	nio_node_t *nio_node;
	mem_node_t *mem_node;
}storage_client_t;

typedef struct STORAGE_SERVICE_T
{
	//
	int network_timeout;
	int sync_beat_millisec;

	tracker_info_t tracker_info;					/*address to connect tracker*/
	int sockfd;							/*listen sockfd*/
	nio_pool_t nio_pool;
	aio_work_t aio_work;
	aeEventLoop *eventLoop;

	int heart_beat_millisec;					/*heart beat*/
	pthread_mutex_t heart_beat_lock;
	heart_beat_t heart_beat;
	return_heart_beat_t return_heart_beat;
	char heart_beat_cmd;

	sync_pool_t sync_pool;
	trunk_file_pool_t trunk_file_pool;

	FILE *storage_config_fp;

	char FINISH_CLOSE_WAIT_BUFF[1024];
	//	
}storage_service_t;

storage_service_t g_storage_service;
#endif
