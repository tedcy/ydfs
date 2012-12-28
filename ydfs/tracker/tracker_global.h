#ifndef TRACKER_GLOBAL_H
#define TRACKER_GLOBAL_H

#define __DEBUG__
#define __TRACKER__

#include "sockopt.h"
#include "share_net.h"
#include "tracker_nio.h"
#include "storage_pool.h"

typedef struct TRACKER_CLIENT_T
{
	nb_data_t data;
	heart_beat_t *heart_beat;
	return_heart_beat_t *return_heart_beat;
	int sockfd;
	storage_address_info_t storage;
	char cmd;	
	char download_filename[24];
	mem_node_t *mem_node;
	nio_node_t *nio_node;
}tracker_client_t;

typedef struct TRACKER_SERVICE_T
{
	int check_storage_pool_millisec;
	int network_timeout;
	float active_to_online;
	
	int sockfd;
	short server_port;
	nio_pool_t nio_pool;
	aeEventLoop *eventLoop;
	storage_pool_t storage_pool;
	FILE *tracker_config_fp;

	char FINISH_CLOSE_WAIT_BUFF[1024];
}tracker_service_t;

tracker_service_t g_tracker_service;

#endif
