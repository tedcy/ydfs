#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "trunk_file.h"
#include "base.h"
#include "log.h"

int init_trunk_file_pool(trunk_file_pool_t *trunk_file_pool)
{
	//trunk_file_node_t *init_node;
	memcpy(trunk_file_pool->trunk_file_name,"0000",sizeof(trunk_file_pool->trunk_file_name));
	memcpy(trunk_file_pool->temp_file_name,"00000000",sizeof(trunk_file_pool->temp_file_name));
	memcpy(trunk_file_pool->real_file_name,"00000000",sizeof(trunk_file_pool->real_file_name));
	
	--trunk_file_pool->trunk_file_name[3];
	--trunk_file_pool->temp_file_name[7];
	--trunk_file_pool->real_file_name[7];

	pthread_mutex_init(&trunk_file_pool->trunk_file_name_lock,NULL);
	pthread_mutex_init(&trunk_file_pool->temp_file_name_lock,NULL);
	pthread_mutex_init(&trunk_file_pool->real_file_name_lock,NULL);

	return 0;
}

trunk_file_node_t* grasp_non_occupied_trunk_file(trunk_file_pool_t *trunk_file_pool,trunk_file_node_t *non_occupied_trunk_file_head,int file_size)
{
	trunk_file_node_t *cur_node;

	cur_node = non_occupied_trunk_file_head;
	if(cur_node != NULL)
	{
		non_occupied_trunk_file_head = cur_node->next;
		return cur_node;
	}
	/*create a new trunk_file*/
	cur_node = malloc(sizeof(trunk_file_node_t));
	memset(cur_node,0,sizeof(trunk_file_node_t));

	pthread_mutex_lock(&trunk_file_pool->trunk_file_name_lock);

	string_add_one(trunk_file_pool->trunk_file_name,4);
	memcpy(cur_node->file_name,trunk_file_pool->trunk_file_name,4);

	pthread_mutex_unlock(&trunk_file_pool->trunk_file_name_lock);
	
	return cur_node;
}

int release_occupied_trunk_file(trunk_file_node_t *non_occupied_trunk_file_head,trunk_file_node_t *trunk_file_node)
{
	if(trunk_file_node->size >= TRUNK_CLOSE_SIZE)
	{
		close(trunk_file_node->fd);
		free(trunk_file_node);
		return 0;
	}
	trunk_file_node->next = non_occupied_trunk_file_head;

	non_occupied_trunk_file_head = trunk_file_node;
	return 0;
}

char* get_temp_file_name(trunk_file_pool_t *trunk_file_pool)
{
	string_add_one(trunk_file_pool->temp_file_name,8);
	
	return trunk_file_pool->temp_file_name;
}

char* get_real_file_name(trunk_file_pool_t *trunk_file_pool)
{
	string_add_one(trunk_file_pool->real_file_name,8);
	
	return trunk_file_pool->real_file_name;
}
