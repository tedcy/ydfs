#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client_api.h"

#define __DEBUG__

int main(int argc,char *argv[])
{
	int count,upload_count,download_count,success_count,success_size,file_size;
	int thread_pool_size;
	char if_continue;
#ifdef __DEBUG__
	strcpy(g_tracker_info.ip,"127.0.0.1");
	g_tracker_info.port = 8000;
#endif
	if(strcmp(argv[1],"--help") == 0)
	{
		printf("\n./ydfs_client uploadTest [dir_name] [upload_times] [client_pool_size]\n"
			"files in dir_name will be upload [upload_times] with a random order,file_name will be write in file \"uploadlog\"\n"
			"\n./ydfs_client downloadTest [dir_name] [client_pool_size]\n"
			"files will be download with an order in file \"uploadlog\"\n");
		printf("\n");
		return 0;
	}
	if(strcmp(argv[1],"upload") == 0)		
	{
		printf("No recommand flag.Use uploadTest replace,are you sure to continue?(y or n)\n");
		if(scanf("%c",&if_continue) <= 0)
			return -1;
		if(if_continue != 'y'&&if_continue != 'Y')
			return -1;
		g_uploadlog_fp = fopen("uploadlog","a");
		if(argc == 4)
		{
			upload_count = atoi(argv[argc-1]);
			for(success_size = success_count = count = 0;count != upload_count;++count)
			{
				if((file_size = upload_file_by_tracker(argv[2])) < 0)
					continue;
				++success_count;
				success_size += file_size;
				printf("success file count: %d,want upload count : %dfile size :%d,total upload size :%d\n",success_count,upload_count,file_size,success_size);
			}
			fclose(g_uploadlog_fp);
			return 0;
		}
		else if(argc == 3)
		{
			if(upload_file_by_tracker_debug(argv[2]) < 0)
			{
				fclose(g_uploadlog_fp);
				return -2;
			}
			return 0;
		}
		printf("input errno! please read the client/ydfs_client.c\n");
	}
	if(strcmp(argv[1],"download") == 0)
	{
		printf("No recommand flag.Use downloadTest replace,are you sure to continue?(y or n)\n");
		if(scanf("%c",&if_continue) <= 0)
			return -1;
		if(if_continue != 'y'&&if_continue != 'Y')
			return -1;
		g_uploadlog_fp = fopen("uploadlog","r");
		char file_name[25];
		if(argc == 2)
		{
			download_count = success_count = success_size = 0;
			for(;;++count)
			{
				if(fgets(file_name,25,g_uploadlog_fp) == NULL)
					return 0;
				file_name[23] = '\0';
				if((file_size = download_file_by_tracker(file_name)) < 0)
					continue ;
				++success_count;
				success_size += file_size;
				printf("file_name :%s,success file count: %d,want upload count : %dfile size :%d,total upload size :%d\n",file_name,success_count,download_count,file_size,success_size);
			}
			fclose(g_uploadlog_fp);
			return 0;
		}
		else if(argc == 3)
		{
			if(download_file_by_tracker_debug(argv[2]) < 0)
			{
				fclose(g_uploadlog_fp);
				return -3;
			}
			return 0;
		}
		printf("input errno! please read the client/ydfs_client.c\n");
	}
	
	if(strcmp(argv[1],"uploadTest") == 0)
	{
		if(argc == 5)
		{
			g_uploadlog_fp = fopen("uploadlog","a");
			upload_count = atoi(argv[argc-2]);
			thread_pool_size = atoi(argv[argc-1]);
			if(upload_test(argv[2],upload_count,thread_pool_size) < 0)
				return -4;
			sleep(100000000);
			fclose(g_uploadlog_fp);
			return 0;
		}
		printf("input errno! please type --help or flag read the client/ydfs_client.c\n");
		return -5;
	}
	
	if(strcmp(argv[1],"downloadTest") == 0)
	{
		if(argc == 3)
		{
			g_uploadlog_fp = fopen("uploadlog","r");
			thread_pool_size = atoi(argv[argc-1]);
			if(download_test(thread_pool_size) < 0)
				return -4;
			sleep(100000000);
			fclose(g_uploadlog_fp);
			return 0;
		}
		printf("input errno! please type --help flag or read the client/ydfs_client.c\n");
		return -6;
	}
	return 0;
}
