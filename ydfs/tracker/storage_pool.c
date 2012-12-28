#include "storage_pool.h"
#include "base.h"
#include "log.h"
#include "tracker_global.h"
#include "share_fun.h"

static void get_send_count_stamp(group_node_t *group,int storage_id);
static void get_recovery_leader_storage(group_node_t *group,int storage_id);

void init_storage_pool()
{
	int group_num;
	group_node_t *group;
	int storage_iter;
	
	for(group_num = 0;group_num != GROUP_MAXSIZE;++group_num)
	{
		group = &g_tracker_service.storage_pool.group[group_num];
		pthread_mutex_init(&group->lock,NULL);
		for(storage_iter = 0;\
			storage_iter != STORAGE_MAXSIZE;\
						++storage_iter)
			pthread_mutex_init(&group->storage[storage_iter].lock,NULL);
	}
		
}

static void get_send_count_stamp(group_node_t *group,int storage_id)
{
	int storage_iter;
	storage_node_t *temp_storage;

	for(storage_iter = 0;storage_iter != STORAGE_MAXSIZE;++storage_iter)
	{
		temp_storage = group->storage + storage_iter;
		memcpy(group->storage[storage_id].send_count_stamps.storage + storage_iter,
			temp_storage->count_stamps.storage + storage_id,8);
	}
	return ;
}

static void get_recovery_leader_storage(group_node_t *group,int storage_id)
{	
	int storage_iter;
	storage_node_t *temp_storage,*best_storage;
	
	best_storage = NULL;
	
	for(storage_iter = 0;storage_iter != STORAGE_MAXSIZE;++storage_iter)
	{
		if(storage_id == storage_iter)
			continue ;
		temp_storage = group->storage + storage_iter;
		if(best_storage == NULL)
			best_storage = temp_storage;
		else if(memcmp(best_storage->count_stamps.storage[storage_id],temp_storage->count_stamps.storage[storage_id],8) < 0)
			best_storage = temp_storage;	
	}	
	group->recovery_leader_sf[storage_id] = best_storage - group->storage;
	return ;
}

void insert_heart_beat(heart_beat_t *hb,in_addr_t ip,return_heart_beat_t *r_hb)
{
	int storage_iter,r_hb_storage_index;
	int s_id;
	group_node_t *group;
	storage_node_t *storage,*temp_storage;

	s_id = (int)hb->storage_id;
	group = &g_tracker_service.storage_pool.group[(int)hb->group_id];
	storage = group->storage + s_id;
	
	/*update storage data and return new_online_storage*/
	if(hb->status == STORAGE_ONLINE)
	{
		r_hb->head.status = STORAGE_ONLINE;
		r_hb->head.storage_id = hb->storage_id;
		
		/*if tracker reboot*/
		LOCK_IF_ERROR("insert_heart_beat",storage->lock);
		if(storage->status == STORAGE_OFFLINE)
		{
			if(group->status != GROUP_ONLINE)
				group->status = GROUP_ONLINE;
			storage->address_info.ip = ip;
			storage->address_info.port = hb->port;
			storage->status = STORAGE_ONLINE;
			memcpy(storage->know_sf,hb->know_sf,sizeof(*storage->know_sf) * STORAGE_MAXSIZE);
			memcpy(group->real_sf,hb->know_sf,sizeof(*storage->know_sf) * STORAGE_MAXSIZE);
			storage->know_sf[s_id] = 1;
			group->real_sf[s_id] = 1;
		}
		
		storage->size = hb->storage_size;
		storage->upload_size = hb->upload_size;
		memcpy(&storage->count_stamps,&(hb->count_stamp),sizeof(hb->count_stamp));
		UNLOCK_IF_ERROR("insert_heart_beat",storage->lock);
		
		/*get new_storage_num*/
		for(storage_iter = 0,r_hb->head.unknow_new_num = r_hb->head.unknow_dea_num = 0;\
			storage_iter != STORAGE_MAXSIZE;++storage_iter)
		{
			LOCK_IF_ERROR("insert_heart_beat",storage->lock);
			if(group->real_sf[storage_iter] == \
				storage->know_sf[storage_iter])
			{
				UNLOCK_IF_ERROR("insert_heart_beat",storage->lock);
				continue ;
			}
			if(group->real_sf[storage_iter] == 1)
			{
				++r_hb->head.unknow_new_num;
				UNLOCK_IF_ERROR("insert_heart_beat",storage->lock);
				continue ;
			}
			++r_hb->head.unknow_dea_num;
			UNLOCK_IF_ERROR("insert_heart_beat",storage->lock);
		}
		if(r_hb->head.unknow_new_num + r_hb->head.unknow_dea_num == 0)
			return ;
		printf("new %d,dead %d\n",r_hb->head.unknow_new_num,r_hb->head.unknow_dea_num);
		fflush(stdout);
		/*malloc space*/
		r_hb->body.new_storage = \
		malloc((r_hb->head.unknow_new_num + r_hb->head.unknow_dea_num) * \
			sizeof(*r_hb->body.new_storage));
		
		r_hb_storage_index = 0;
		if(r_hb->head.unknow_new_num != 0)
		{
			/*get new_online_storage address and storage_id*/
			for(storage_iter = 0;storage_iter != STORAGE_MAXSIZE;++storage_iter)
			{
				LOCK_IF_ERROR("insert_heart_beat",storage->lock);
				if(group->real_sf[storage_iter] != \
					storage->know_sf[storage_iter])
				{
					if(group->real_sf[storage_iter] == 1)
					{		
						if(group->recovery_leader_sf[storage_iter] == s_id)
						{
							r_hb->body.new_storage[r_hb_storage_index].if_need_recovery = 1;
							group->recovery_leader_sf[storage_iter] = -1;
						}
						else
							r_hb->body.new_storage[r_hb_storage_index].if_need_recovery = 0;
						r_hb->body.new_storage[r_hb_storage_index].storage_id = storage_iter;
						memcpy(&r_hb->body.new_storage[r_hb_storage_index].storage_address_info,\
							&group->storage[storage_iter].address_info,\
							sizeof(storage_address_info_t));
						++r_hb_storage_index;
					}
				}
				UNLOCK_IF_ERROR("insert_heart_beat",storage->lock);
			}
		}
		if(r_hb->head.unknow_dea_num != 0)
		{
			/*get dead_online_storage address and storage_id*/
			for(storage_iter = 0;storage_iter != STORAGE_MAXSIZE;++storage_iter)
			{
				if(group->real_sf[storage_iter] != \
					storage->know_sf[storage_iter])
				{
					if(group->real_sf[storage_iter] == 0)
					{
						r_hb->body.new_storage[r_hb_storage_index].storage_id = storage_iter;
						memcpy(&r_hb->body.new_storage[r_hb_storage_index].storage_address_info,\
							&group->storage[storage_iter].address_info,\
							sizeof(storage_address_info_t));
						++r_hb_storage_index;
					}
				}
			}
		}
		return ;
	}
	if(hb->status == STORAGE_ACTIVE)
	{
		r_hb->head.status = STORAGE_ACTIVE;
		r_hb->head.storage_id = hb->storage_id;
		memcpy(&storage->count_stamps,&(hb->count_stamp),sizeof(hb->count_stamp));
	
		/*if tracker reboot*/
		LOCK_IF_ERROR("insert_heart_beat",storage->lock);
		if(storage->status == STORAGE_OFFLINE)
		{
			if(group->status != GROUP_ONLINE)
				group->status = GROUP_ONLINE;
			storage->address_info.ip = ip;
			storage->address_info.port = hb->port;
			storage->status = STORAGE_ACTIVE;
			storage->know_sf[s_id] = 1;
			group->real_sf[s_id] = 1;
		}
		else
		{
			/*if storage is turnned from STORAGE_ACTIVE to STORAGE_ONLINE by tracker*/
			if(storage->status == STORAGE_ONLINE)
			{
				r_hb->head.status = STORAGE_ONLINE;
			}
		}
		storage->size = hb->storage_size;	
		storage->upload_size = hb->upload_size;
		UNLOCK_IF_ERROR("insert_heart_beat",storage->lock);
		return ;
	}
	if(hb->status == STORAGE_RECOVERY)
	{
		r_hb->head.status = STORAGE_RECOVERY;
		r_hb->head.storage_id = hb->storage_id;
		memcpy(&storage->count_stamps,&(hb->count_stamp),sizeof(hb->count_stamp));
	
		/*if tracker reboot*/
		LOCK_IF_ERROR("insert_heart_beat",storage->lock);
		if(storage->status == STORAGE_OFFLINE)
		{
			if(group->status != GROUP_ONLINE)
				group->status = GROUP_ONLINE;
			storage->address_info.ip = ip;
			storage->address_info.port = hb->port;
			storage->status = STORAGE_RECOVERY;
			storage->know_sf[s_id] = 1;
			group->real_sf[s_id] = 1;
		}
		storage->size = hb->storage_size;	
		storage->upload_size = hb->upload_size;
		UNLOCK_IF_ERROR("insert_heart_beat",storage->lock);
		return ;
	}
	/*if storage be STORAGE_RECOVERYED,means it is recoveryed,so get the size and it will be STORAGE_ACTIVE*/
	if(hb->status == STORAGE_RECOVERYED)
	{
		r_hb->head.status = STORAGE_ACTIVE;
		memcpy(&storage->count_stamps,&(hb->count_stamp),sizeof(hb->count_stamp));
		
		/*if tracker reboot*/
		if(storage->status == STORAGE_OFFLINE)
		{
			if(group->status != GROUP_ONLINE)
				group->status = GROUP_ONLINE;
			storage->address_info.ip = ip;
			storage->address_info.port = hb->port;
			storage->status = STORAGE_ACTIVE;
		}
		
		storage->size = hb->storage_size;	
		storage->upload_size = hb->upload_size;
		storage->status = STORAGE_ACTIVE;
		group->real_sf[s_id] = 1;

		r_hb->head.status = STORAGE_ACTIVE;
		return ;
	}
	/*if storage is a new one,get a space and other storage will push all of it's data to this storage*/
	if(hb->status == STORAGE_OFFLINE)
	{
		/*if the group have no storage*/
		LOCK_IF_ERROR("insert_heart_beat",group->lock);
		if(group->status == GROUP_OFFLINE)
		{
			UNLOCK_IF_ERROR("insert_heart_beat",group->lock);

			temp_storage = group->storage + 0;
			LOCK_IF_ERROR("insert_heart_beat",temp_storage->lock);
			temp_storage->address_info.ip = ip;
			temp_storage->address_info.port = hb->port;
			temp_storage->status = STORAGE_ONLINE;
			temp_storage->know_sf[0] = 1;
			group->real_sf[0] = 1;
	
			group->recovery_leader_sf[0] = -1;
			UNLOCK_IF_ERROR("insert_heart_beat",temp_storage->lock);

			LOCK_IF_ERROR("insert_heart_beat",group->lock);
			group->status = GROUP_ONLINE;
			UNLOCK_IF_ERROR("insert_heart_beat",group->lock);

			r_hb->head.storage_id = 0;
			r_hb->head.unknow_new_num = 0;
			r_hb->head.status = STORAGE_ONLINE;
			
			return ;
		}
		UNLOCK_IF_ERROR("insert_heart_beat",group->lock);
		/*if the storage is STORAGE_DEAD and now alive*/
		if(hb->storage_id != -1)
		{
			storage->address_info.ip = ip;
			storage->address_info.port = hb->port;
			storage->status = STORAGE_RECOVERY;

			r_hb->head.storage_id = hb->storage_id;
			r_hb->head.status = STORAGE_RECOVERY;

			storage->know_sf[s_id] = 1;
			group->real_sf[s_id] = 1;

			get_recovery_leader_storage(group,s_id);
			get_send_count_stamp(group,s_id);
			return ;
		}
		for(storage_iter = 0;storage_iter != STORAGE_MAXSIZE;++storage_iter)
		{
			temp_storage = group->storage + storage_iter;
			LOCK_IF_ERROR("insert_heart_beat",temp_storage->lock);
			if(temp_storage->status == STORAGE_OFFLINE)
			{
				temp_storage->address_info.ip = ip;
				temp_storage->address_info.port = hb->port;
				temp_storage->status = STORAGE_ACTIVE;
				temp_storage->know_sf[storage_iter] = 1;
				group->real_sf[storage_iter] = 1;
				
				group->recovery_leader_sf[storage_iter] = -1;
				UNLOCK_IF_ERROR("insert_heart_beat",temp_storage->lock);

				r_hb->head.storage_id = storage_iter;
				r_hb->head.status = STORAGE_ACTIVE;

				return ;
			}
			UNLOCK_IF_ERROR("insert_heart_beat",temp_storage->lock);
		}
	}
	return ;
}

int delete_heart_beat(int group_id,int storage_id)
{
	return 0;
}

int dead_heart_beat(int group_id,int storage_id)
{
	group_node_t *group;
	storage_node_t *storage;
	
	group = &g_tracker_service.storage_pool.group[group_id];
	storage = group->storage + storage_id;

	if(storage->status != STORAGE_OFFLINE&&\
		storage->status != STORAGE_DEAD)
	{
		memset(storage->know_sf,0,STORAGE_MAXSIZE);
		memset(&storage->count_stamps,0,sizeof(count_stamp_t));
		memset(&storage->send_count_stamps,0,sizeof(count_stamp_t));
		storage->size = 0;
		storage->upload_size = 0;
		group->real_sf[storage_id] = 0;
		group->storage[storage_id].know_sf[storage_id] = 0;
	}
	group->storage[storage_id].status = STORAGE_DEAD;
	return 0;
}

void manage_storage_pool()
{
	int group_num,storage_iter;
	group_node_t *group;
	storage_node_t *temp_storage;
	char inet_buff[INET_ADDRSTRLEN];
	
	for(group_num = 0;group_num != GROUP_MAXSIZE;++group_num)
	{
		group = &g_tracker_service.storage_pool.group[group_num];
		LOCK_IF_ERROR("manage_storage_pool",group->lock);
		if(group->status == GROUP_OFFLINE)
		{
			UNLOCK_IF_ERROR("manage_storage_pool",group->lock);
			continue;
		}
		UNLOCK_IF_ERROR("manage_storage_pool",group->lock);
		group->group_size = 0;
		group->group_upload_size = 0;
		group->best_storage = NULL;
		printf("group_name %d\n",group_num);
		
		for(storage_iter = 0;\
			storage_iter != STORAGE_MAXSIZE;\
						++storage_iter)
		{
			temp_storage = group->storage + storage_iter;
			LOCK_IF_ERROR("manage_storage_pool",temp_storage->lock);
			if(temp_storage->status != STORAGE_OFFLINE)
			{
				printf("storage_id %d\tip %s:%d\tsize %lld,upload_size %lld\n"
					"count_stamp %s\t%s\t%s ",\
					storage_iter,\
					inet_ntop(AF_INET,&temp_storage->address_info.ip,inet_buff,INET_ADDRSTRLEN),\
					temp_storage->address_info.port,\
					temp_storage->size,temp_storage->upload_size,
					temp_storage->count_stamps.storage[0],temp_storage->count_stamps.storage[1],
					temp_storage->count_stamps.storage[2]);
				if(temp_storage->status == STORAGE_ONLINE)
				{
					if(group->best_storage == NULL||temp_storage->upload_size < group->best_storage->upload_size)
						group->best_storage = temp_storage;	
					group->group_size += temp_storage->size;
					group->group_upload_size += temp_storage->upload_size;
					printf("\tONLINE\n");
					UNLOCK_IF_ERROR("manage_storage_pool",temp_storage->lock);
					continue ;
				}
				if(temp_storage->status == STORAGE_ACTIVE)
				{
					group->group_size += temp_storage->size;
					printf("\tACTIVE\n");
					UNLOCK_IF_ERROR("manage_storage_pool",temp_storage->lock);
					continue ;
				}
				if(temp_storage->status == STORAGE_RECOVERY)
				{
					group->group_size += temp_storage->size;
					printf("\tRECVOERY\n");
					UNLOCK_IF_ERROR("manage_storage_pool",temp_storage->lock);
					continue ;
				}
				if(temp_storage->status == STORAGE_RECOVERYED)
				{
					group->group_size += temp_storage->size;
					printf("\tRECVOERYED\n");
					UNLOCK_IF_ERROR("manage_storage_pool",temp_storage->lock);
					continue ;
				}
				if(temp_storage->status == STORAGE_DEAD)
				{
					printf("\tDEAD\n");
					UNLOCK_IF_ERROR("manage_storage_pool",temp_storage->lock);
					continue ;
				}
			}
			UNLOCK_IF_ERROR("manage_storage_pool",temp_storage->lock);
		}
		printf("group_size %lld,group_upload_size %lld\n",group->group_size,group->group_upload_size);
		/*if storage over the active_to_online*group_upload_size,turn it into STORAGE_ONLINE*/
		for(storage_iter = 0;\
			storage_iter != STORAGE_MAXSIZE;\
						++storage_iter)
		{
			temp_storage = group->storage + storage_iter;
			LOCK_IF_ERROR("manage_storage_pool",temp_storage->lock);
			if(temp_storage->status == STORAGE_ACTIVE)
			{
				if(temp_storage->size >= group->group_upload_size*g_tracker_service.active_to_online)
					temp_storage->status = STORAGE_ONLINE;
			}
			UNLOCK_IF_ERROR("manage_storage_pool",temp_storage->lock);
		}
		LOCK_IF_ERROR("manage_storage_pool",group->lock);
		if(g_tracker_service.storage_pool.best_group == NULL||group->group_size < g_tracker_service.storage_pool.best_group->group_size)
				g_tracker_service.storage_pool.best_group = group;
		UNLOCK_IF_ERROR("manage_storage_pool",group->lock);
	}
}

void get_best_storage(storage_address_info_t *storage_address)
{
	LOCK_IF_ERROR("get_best_storage",g_tracker_service.storage_pool.group->lock);
	if(g_tracker_service.storage_pool.best_group != NULL)
		if(g_tracker_service.storage_pool.best_group->best_storage != NULL)
		{
			memcpy(storage_address,&g_tracker_service.storage_pool.best_group->best_storage->address_info,sizeof(*storage_address));
		}
	UNLOCK_IF_ERROR("get_best_storage",g_tracker_service.storage_pool.group->lock);

	return ;
}

void find_download_storage(char *file_name,storage_address_info_t *storage_address)
{
	int storage_iter;
	int group_id;
	int storage_id;
	group_node_t *group;
	storage_node_t *storage,*temp_storage,*best_storage;

	group_id = string_to_int(file_name+13,2);
	storage_id = string_to_int(file_name+12,1);

	group = g_tracker_service.storage_pool.group + group_id;
	storage = group->storage + storage_id;	
	if(storage->status != STORAGE_DEAD)
		best_storage = storage;
	else best_storage = NULL;

	for(storage_iter = 0;storage_iter != STORAGE_MAXSIZE;++storage_iter)
	{
		temp_storage = group->storage + storage_iter;
		if(temp_storage->status == STORAGE_DEAD)
			continue;
		if(temp_storage->status == STORAGE_OFFLINE)
			continue;
		/*if file count_stamp <= group[group_id].storage[any_storage_id].count_stamp[storage_id],then the any_storage_id has the file needed*/
		if(memcmp(file_name,temp_storage->count_stamps.storage[storage_id],8) <= 0)
		{
			/*then choice the max size storage to be used(for test)*/
			if(best_storage == NULL)
				best_storage = temp_storage;
			else if(best_storage->upload_size < temp_storage->upload_size)
					best_storage = temp_storage;
		}
	}

	storage_address->ip = best_storage->address_info.ip;
	storage_address->port = best_storage->address_info.port;
	
	return ;
}
