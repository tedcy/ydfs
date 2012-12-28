#ifndef TRACKER_LIST_H
#define TRACKER_LIST_H

#include <arpa/inet.h>
#include <pthread.h>

#include "heart_beat.h"

typedef struct STORAGE_NODE_T
{
	long long size;				//storage size
	long long upload_size;
	storage_address_info_t address_info;
	count_stamp_t count_stamps;
	count_stamp_t send_count_stamps;
	pthread_mutex_t lock;
	char status;
	char know_sf[STORAGE_MAXSIZE];
}storage_node_t;

typedef struct GROUP_NODE_T
{
	storage_node_t storage[STORAGE_MAXSIZE]; 
	storage_node_t *best_storage;
	char real_sf[STORAGE_MAXSIZE];
	char recovery_leader_sf[STORAGE_MAXSIZE];
	int storage_num;
	long long group_size;
	long long group_upload_size;
	char status;
	pthread_mutex_t lock;
}group_node_t;

typedef struct STORAGE_POOL_T
{
	group_node_t group[GROUP_MAXSIZE];
	group_node_t *best_group;
}storage_pool_t;

void init_storage_pool();
void insert_heart_beat(heart_beat_t *heart_beat,in_addr_t ip,return_heart_beat_t *return_heart_beat);
int delete_heart_beat(int group_id,int storage_id);
int dead_heart_beat(int group_id,int storage_id);
void manage_storage_pool();
void get_best_storage(storage_address_info_t *address_info);
void find_download_storage(char *file_name,storage_address_info_t *storage_address);

#endif
