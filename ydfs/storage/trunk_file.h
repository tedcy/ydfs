#ifndef TRUNK_FILE_H
#define TRUNK_FILE_H

#include <pthread.h>

//#define TRUNK_MAX_SIZE 102400
#define TRUNK_MAX_SIZE 1048576
//when the trunk_size over 1M-20K close it
#define TRUNK_CLOSE_SIZE 1028096

typedef struct TRUNK_FILE_NODE_T
{
	int fd;
	int size;
	/*4 bit trunk_name,1 bit storage_id*/
	char file_name[6];
	struct TRUNK_FILE_NODE_T *next;
}trunk_file_node_t;


typedef struct TRUNK_FILE_POOL_T
{
	char trunk_file_name[5];
	char temp_file_name[9];
	char real_file_name[9];
	int max_thread_num;
	pthread_mutex_t trunk_file_name_lock,temp_file_name_lock,real_file_name_lock;
}trunk_file_pool_t;

int init_trunk_file_pool(trunk_file_pool_t *trunk_file_pool);
trunk_file_node_t* grasp_non_occupied_trunk_file(trunk_file_pool_t *trunk_file_pool,trunk_file_node_t *non_occupied_trunk_file_head,int file_size);
int release_occupied_trunk_file(trunk_file_node_t *non_occupied_trunk_file_head,trunk_file_node_t *trunk_file_node);
char* get_temp_file_name(trunk_file_pool_t *trunk_file_pool);
char* get_real_file_name(trunk_file_pool_t *trunk_file_pool);

#endif
