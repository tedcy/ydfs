#ifndef STORAGE_CONFIG_H
#define STORAGE_CONFIG_H

#include <stdio.h>
#include <string.h>

#include "storage_global.h"
#include "log.h"

#define STORAGE_CONFIG_NAME 	"storage.config"
#define BUF_LINE_SIZE 		1024
#define VALUE_START_CHAR	'='

#define load_key_value(key_name,key,value)\
	do\
	{\
		if(memcmp(BUF_LINE,key_name,strlen(key_name)) == 0)\
		{\
			if((value_start = strchr(BUF_LINE,VALUE_START_CHAR)) != NULL)\
			{\
				while(*value_start++ != ' ');\
				key = value;\
				goto load_value_break;\
			}\
			else\
			{\
				logConfig(key_name);\
				return -1;\
			}\
		}\
	}while(0)

#define load_key_string(key_name,key)\
	do\
	{\
		if(memcmp(BUF_LINE,key_name,strlen(key_name)) == 0)\
		{\
			if((value_end = value_start = strchr(BUF_LINE,VALUE_START_CHAR)) != NULL)\
			{\
				while(*value_start++ != ' ');\
				while(*value_end++ != '\n');\
				memcpy(key,value_start,value_end-1-value_start);\
				goto load_value_break;\
			}\
			else\
			{\
				logConfig(key_name);\
				return -1;\
			}\
		}\
	}while(0)
static int load_storage_value();

static int load_storage_config()
{
	g_storage_service.storage_config_fp = fopen(STORAGE_CONFIG_NAME,"r");
	if(load_storage_value() < 0)
	{
		fclose(g_storage_service.storage_config_fp);
		return -1;
	}
	fclose(g_storage_service.storage_config_fp);
	return 0;
}

static int load_storage_value()
{
	char BUF_LINE[BUF_LINE_SIZE];	
	char *value_start = NULL;
	char *value_end = NULL;
	
	while(fgets(BUF_LINE,BUF_LINE_SIZE,g_storage_service.storage_config_fp) != NULL)
	{
		if(BUF_LINE[0] == '#'||BUF_LINE[0] == ' ')
			continue;
		load_key_value("heart_beat_millisec",\
			g_storage_service.heart_beat_millisec,strtol(value_start,&value_end,10)*1000);

		load_key_value("sync_beat_millisec",\
			g_storage_service.sync_beat_millisec,strtol(value_start,&value_end,10)*1000);

		load_key_value("network_timeout",\
			g_storage_service.network_timeout,strtol(value_start,&value_end,10));

		load_key_string("tracker_info_address_ip",g_storage_service.tracker_info.ip);

		load_key_value("tracker_info_address_port",\
			g_storage_service.tracker_info.port,strtol(value_start,&value_end,10));

		load_key_value("network_thread_num",\
			g_storage_service.nio_pool.max_thread_num,strtol(value_start,&value_end,10));

		load_key_value("storage_port_of_myself",\
			g_storage_service.heart_beat.port,strtol(value_start,&value_end,10));

		load_key_value("group_id_of_myself",\
			g_storage_service.heart_beat.group_id,strtol(value_start,&value_end,10));

		load_key_value("check_count_stamp_millisec",\
			g_storage_service.sync_pool.check_count_stamp_millisec,strtol(value_start,&value_end,10)*1000);

		load_key_value("sync_connect_millisec",\
			g_storage_service.sync_pool.sync_connect_millisec,strtol(value_start,&value_end,10)*1000);

		load_key_value("storage_id_of_myself",\
			g_storage_service.heart_beat.storage_id,strtol(value_start,&value_end,10));
load_value_break:
		;
	}
	g_storage_service.trunk_file_pool.max_thread_num = g_storage_service.nio_pool.max_thread_num;
	g_storage_service.sync_pool.check_recovery_count_stamp_millisec = g_storage_service.sync_pool.check_count_stamp_millisec;
	return 0;
}

#endif
