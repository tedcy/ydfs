#ifndef TRACKER_CONFIG_H
#define TRACKER_CONFIG_H

#include <stdio.h>
#include <string.h>

#include "tracker_global.h"
#include "log.h"

#define TRACKER_CONFIG_NAME 	"tracker.config"
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
static int load_tracker_value();

static int load_tracker_config()
{
	g_tracker_service.tracker_config_fp = fopen(TRACKER_CONFIG_NAME,"r");
	if(load_tracker_value() < 0)
	{
		fclose(g_tracker_service.tracker_config_fp);
		return -1;
	}
	fclose(g_tracker_service.tracker_config_fp);
	return 0;
}
static int load_tracker_value()
{
	char BUF_LINE[BUF_LINE_SIZE];	
	char *value_start = NULL;
	char *value_end = NULL;
	
	while(fgets(BUF_LINE,BUF_LINE_SIZE,g_tracker_service.tracker_config_fp) != NULL)
	{
		if(BUF_LINE[0] == '#'||BUF_LINE[0] == ' ')
			continue;
		load_key_value("network_timeout",\
			g_tracker_service.network_timeout,strtol(value_start,&value_end,10));

		load_key_value("network_thread_num",\
			g_tracker_service.nio_pool.max_thread_num,strtol(value_start,&value_end,10));

		load_key_value("tracker_port_of_myself",\
			g_tracker_service.server_port,strtol(value_start,&value_end,10));

		load_key_value("check_storage_pool_millisec",\
			g_tracker_service.check_storage_pool_millisec,strtol(value_start,&value_end,10)*1000);
		load_key_value("active_to_online",\
			g_tracker_service.active_to_online,strtol(value_start,&value_end,10)/100);
load_value_break:
		;
	}
	return 0;
}

#endif
