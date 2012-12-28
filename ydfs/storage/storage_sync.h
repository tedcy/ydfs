#ifndef STORAGE_SYNC_H
#define STORAGE_SYNC_H

#include <pthread.h>

#include "sockopt.h"
#include "heart_beat.h"
#include "storage_nio.h"

#define BINLOG_NAME "binlog"
#define SYNCLOG_NAME "synclog"

struct SYNC_POOL_T;

typedef struct SYNC_NODE_T
{
	int sockfd;
	int stop_fd[2];
	int storage_id;
	int binlog_read_count;
	int synclog_read_count;
	int temp_binlog_read_count;
	int temp_synclog_read_count;
	char if_need_recovery;
	char temp_file_name[9];
	pthread_t node_pid;
	storage_address_info_t sync_address_info;

	nio_node_t nio_node;
	struct SYNC_POOL_T *sync_pool; //father pool
}sync_node_t;

typedef struct SYNC_POOL_T
{
	sync_node_t sync_node[STORAGE_MAXSIZE];
	count_stamp_t *count_stamp;
	count_stamp_t send_count_stamp;
	char *real_count_stamp_file_name;
	FILE *binlog_fp;
	FILE *synclog_fp;
	pthread_mutex_t binlog_read_count_lock;
	pthread_mutex_t synclog_read_count_lock;
	char konw_active_and_online_storage_flag[STORAGE_MAXSIZE];
	int check_count_stamp_millisec;
	int check_recovery_count_stamp_millisec;
	int sync_connect_millisec;
	char sync_start_cmd;
}sync_pool_t;

int init_sync_pool(sync_pool_t *sync_pool,count_stamp_t *count_stamp,char *real_count_stamp_file_name);
void do_sync_from_storage(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);

int sync_connect(struct aeEventLoop *eventLoop, long long id, void *clientData);

int init_new_sync_thread(sync_pool_t *sync_pool,int storage_id,storage_address_info_t *storage_address_info,char if_need_recovery);
int destory_new_sync_thread(sync_pool_t *sync_pool,int storage_id);

#endif
