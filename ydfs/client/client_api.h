#ifndef CLIENT_API_H
#define CLIENT_API_H

#include <pthread.h>
#include <time.h>

typedef struct TRACKER_INFO_T
{
	char ip[32];
	int port;	
}tracker_info_t;//连接tracker所用

typedef struct UPLOAD_TEST_INFO_T
{
	char **file_names;
	int name_counts;
	int success_count;
	int fail_count;
	int all_count;
	long long upload_size;
	int have_upload_count;
	int upload_count;
	time_t start_time;
	int all_sec;
}upload_test_info_t;

typedef struct DOWNLOAD_TEST_INFO_T
{
	char file_names[100000][25];
	int name_counts;
	int success_count;
	int fail_count;
	int all_count;
	long long download_size;
	int have_download_count;
	int download_count;
	time_t start_time;
	int all_sec;
}download_test_info_t;

tracker_info_t g_tracker_info;
upload_test_info_t 	g_upload_test_info;
download_test_info_t 	g_download_test_info;
extern FILE *g_uploadlog_fp;

int upload_file_by_tracker(char *file_name);
int upload_file_by_tracker_debug(char *file_name);
int download_file_by_tracker(char *file_name);
int download_file_by_tracker_debug(char *file_name);
int upload_test(char *dir_name,int upload_count,int thread_count);
int download_test(int thread_count);

#endif
