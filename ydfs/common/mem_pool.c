#include <stdlib.h>

#include "mem_pool.h"
#include "log.h"

int do_mem_pool(mem_pool_t *mem_pool)
{
	int index;
	mem_node_t *cur_node;
	mem_node_t *old_freed_head;
	char *cur_buf;

	mem_pool->buf[mem_pool->cb_index] = malloc(mem_pool->max_node_num *(mem_pool->node_size + \
			sizeof(mem_node_t)));
	
	if(mem_pool->buf[mem_pool->cb_index] == NULL)
	{
		logError(	"file :"__FILE__",line :%d,"\
			"do_mem_pool failed,no more memory",__LINE__);
		return -1;
	}

	/*the old list head will be the tail->next after the new one*/
	old_freed_head = mem_pool->freed_head;

	mem_pool->freed_head = cur_node = mem_pool->buf[mem_pool->cb_index];
	cur_buf = (char *)mem_pool->buf[mem_pool->cb_index] + mem_pool->max_node_num * sizeof(mem_node_t);

	/*
	 *the last node,node->next must be the old list head,so index from 0 to max_noe_num-1;
	 */ 
	for(index = 0;index != mem_pool->max_node_num - 1;++index)
	{
		cur_node->buf = cur_buf;	
		cur_node->next = (cur_node + 1);

		++cur_node;
		cur_buf += mem_pool->node_size;
	}
	cur_node->buf = cur_buf;
	cur_node->next = old_freed_head;

	/*point to the new buf which will be malloc*/
	++mem_pool->cb_index;
	return 0;
}

mem_node_t *get_freed_mem(mem_pool_t *mem_pool)
{
	if(mem_pool->freed_head == NULL)
	{
		if(resize_mem_pool(mem_pool) < 0)
			return NULL;
	}
	mem_node_t *cur_node;
	
	cur_node = mem_pool->freed_head;
	mem_pool->freed_head = mem_pool->freed_head->next;
	return cur_node;
}

void release_mem(mem_pool_t *mem_pool,mem_node_t *mem_node)
{
	mem_node->next = mem_pool->freed_head;
	mem_pool->freed_head = mem_node;
	return ;
}
