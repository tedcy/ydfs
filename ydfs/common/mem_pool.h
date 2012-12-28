#ifndef MEM_POOL_H
#define MEM_POOL_H

#define MAX_BUF_POINT_COUNT 100

typedef struct MEM_NODE_T
{
	void *buf;
	struct MEM_NODE_T *next;
}mem_node_t;

typedef struct MEM_POOL_T
{
	void *buf[MAX_BUF_POINT_COUNT];
	/*point to the cur buf index*/
	int cb_index;
	mem_node_t *freed_head;
	long long node_size;
	long long max_node_num;
}mem_pool_t;

#define init_mem_pool(mem_pool) do_mem_pool(mem_pool)
#define resize_mem_pool(mem_pool) do_mem_pool(mem_pool)

int do_mem_pool(mem_pool_t *mem_pool);
mem_node_t *get_freed_mem(mem_pool_t *mem_pool);
void release_mem(mem_pool_t *mem_pool,mem_node_t *mem_node);

#endif
