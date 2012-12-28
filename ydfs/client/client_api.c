#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

#include "log.h"
#include "heart_beat.h"
#include "client_api.h"
#include "sockopt.h"
#include "tracker_proto.h"
#include "storage_proto.h"
#include "base.h"
#include "thread_pool.h"

FILE *g_uploadlog_fp;

static void get_upload_file_names(char *dir_name);
static void get_download_file_names();
static void *upload_thread(void *arg);
static void upload_end();
static void *download_thread(void *arg);
static void download_end();

/*
 * upload file api
 * (1)  connect_the_tracker_by_ip
 *
 * (2)  send data 1 byte ,		(type: char) cmd = UPLOAD_GET_STORAGE(value = 1)
 * (3)  recv data 8 bytes
 * 		 the first  4 bytes	(type :in_addr_t     ) ip
 * 		 the second 2 bytes	(type :unsigned short) port
 * (4)  close_tracker_socket
 *
 * (5)  connect_the_storage_by $(ip) and $(port) just_recv
 *
 * (6)  send data 1 byte ,		(type: char) cmd = UPLOAD_TO_STORAGE(value = 1)
 * (7)  send data 4 bytes,		(type: int ) file_size
 * (8)  send data $(file_size) bytes 	in the file
 * (9)  recv data 24 bytes,save it,this is the key to download file
 *
 * (10) close_storage_socket
 */

int upload_file_by_tracker(char *file_name)
{
	int tracker_sock,storage_sock;
	int file_size;
	storage_address_info_t best_storage;
	char cmd;
	
	//连接tracker获得最好的storage信息

	if((tracker_sock = connect_ip(g_tracker_info.ip,g_tracker_info.port)) < 0)
	{
		close(tracker_sock);
		return -1;
	}
	
	tcpsetnodelay(tracker_sock,3600);

	cmd = UPLOAD_GET_STORAGE;
	if(sock_send_data(&cmd,sizeof(cmd),tracker_sock) < 0)
	{
		close(tracker_sock);
		return -2;
	}

	if(sock_recv_data(&best_storage,sizeof(best_storage),tracker_sock) < 0)
	{
		close(tracker_sock);
		return -3;
	}
	close(tracker_sock);
	
	//无服务器

	if(best_storage.ip == 0&&best_storage.port == 0)
		return -4;

	char inet_buf[INET_ADDRSTRLEN];
	if((storage_sock = connect_ip(inet_ntop(AF_INET,&best_storage.ip,inet_buf,INET_ADDRSTRLEN),best_storage.port)) < 0)
	{
		close(storage_sock);
		return -5;
	}
	
	tcpsetnodelay(storage_sock,3600);


	cmd = UPLOAD_TO_STORAGE;
	if(sock_send_data(&cmd,sizeof(cmd),storage_sock) < 0)
	{
		close(storage_sock);
		return -6;
	}
	
	if((file_size = sock_send_file(file_name,storage_sock)) < 0)
	{
		close(storage_sock);	
		return -7;
	}

	char recv_file_name[24];
	memset(recv_file_name,'0',24);
	if(sock_recv_data(recv_file_name,24,storage_sock) < 0)
	{
		close(storage_sock);
		return -8;
	}
	recv_file_name[23] = '\n';

	logDebug("%s",recv_file_name);
	write_file_log(recv_file_name,24,g_uploadlog_fp);

	close(storage_sock);
	return file_size;
}

int upload_file_by_tracker_debug(char *file_name)
{
	int tracker_sock,storage_sock;
	int file_size;
	storage_address_info_t best_storage;
	char cmd;
	
	//连接tracker获得最好的storage信息

	logDebug("*connect tracker ",...);
	//fflush(stdout);
	if((tracker_sock = connect_ip(g_tracker_info.ip,g_tracker_info.port)) < 0)
	{
		close(tracker_sock);
		return -1;
	}
	logDebug("success!\n",...);
	//fflush(stdout);
	
	tcpsetnodelay(tracker_sock,3600);
	
	logDebug("*send UPLOAD_GET_STORAGE ",...);
	//fflush(stdout);
	cmd = UPLOAD_GET_STORAGE;
	if(sock_send_data(&cmd,sizeof(cmd),tracker_sock) < 0)
	{
		close(tracker_sock);
		return -2;
	}	
	logDebug("success!\n",...);
	//fflush(stdout);
	
	logDebug("*recv best_storage ",...);
	//fflush(stdout);
	if(sock_recv_data(&best_storage,sizeof(best_storage),tracker_sock) < 0)
	{
		close(tracker_sock);
		return -3;
	}
	logDebug("success!\n",...);
	//fflush(stdout);
	
	close(tracker_sock);
	//无服务器

	logDebug("*check best_storage exists ",...);
	//fflush(stdout);
	if(best_storage.ip == 0&&best_storage.port == 0)
		return -4;
	logDebug("success!\n",...);
	//fflush(stdout);

	char inet_buf[INET_ADDRSTRLEN];
	logDebug("connect storage! %s:%d",\
		inet_ntop(AF_INET,&best_storage.ip,inet_buf,INET_ADDRSTRLEN),\
			best_storage.port);
	//fflush(stdout);
	if((storage_sock = connect_ip(inet_buf,best_storage.port)) < 0)
	{
		close(storage_sock);
		return -5;
	}
	logDebug("success!\n",...);
	//fflush(stdout);
	
	tcpsetnodelay(storage_sock,3600);
	
	logDebug("*send cmd UPLOAD_TO_STORAGE ",...);
	//fflush(stdout);
	cmd = UPLOAD_TO_STORAGE;
	if(sock_send_data(&cmd,sizeof(cmd),storage_sock) < 0)
	{
		close(storage_sock);
		return -6;
	}
	logDebug("success!\n",...);
	//fflush(stdout);
	
	logDebug("*upload file ",...);
	//fflush(stdout);
	if((file_size = sock_send_file(file_name,storage_sock)) < 0)
	{
		close(storage_sock);
		return -7;
	}
	logDebug("success!\n",...);
	//fflush(stdout);
	
	logDebug("*recv file_name",...);
	char recv_file_name[24];
	if(sock_recv_data(recv_file_name,24,storage_sock) < 0)
	{
		close(storage_sock);
		return -8;
	}
	recv_file_name[23] = '\n';
	logDebug("sucess!\n",...);

	logDebug("*write file_name %s",recv_file_name);
	write_file_log(recv_file_name,24,g_uploadlog_fp);
	logDebug("sucess!\n",...);
	close(storage_sock);
	
	return file_size;
}
/*
 * download file api
 * (1)  connect_the_tracker_by_ip
 *
 * (2)  send data 1 byte ,		(type: char ) cmd = DOWNLOAD_GET_STORAGE(value = 2)
 * (3)	send data 24 byte,		(type: char * ) file_name
 * (4)  recv data 8 bytes
 * 		 the first  4 bytes	(type :in_addr_t     ) ip
 * 		 the second 2 bytes	(type :unsigned short) port
 * (5)  close_tracker_socket
 *
 * (6)  connect_the_storage_by $(ip) and $(port) just_recv
 *
 * (7)  send data 1 byte ,		(type: char) cmd = DOWNLOAD_FORM_STORAGE(value = 2)
 * (8)  send data 24 bytes,		(type: char * ) file_name
 * (9)  calculate file_size from file_name see the string_to_int(file_name+19,4)
 * (10) recv data $(file_size) bytes of the file
 *
 * (11) close_storage_socket
 */
int download_file_by_tracker(char *file_name)
{
	int tracker_sock,storage_sock;
	storage_address_info_t which_storage;
	char cmd;
	
	//连接tracker获得最好的storage信息
	if((tracker_sock = connect_ip(g_tracker_info.ip,g_tracker_info.port)) < 0)
	{
		close(tracker_sock);
		return -1;
	}

	tcpsetnodelay(tracker_sock,3600);
	
	cmd = DOWNLOAD_GET_STORAGE;
	if(sock_send_data(&cmd,sizeof(cmd),tracker_sock) < 0)
	{
		close(tracker_sock);
		return -2;
	}
	
	if(sock_send_data(file_name,24,tracker_sock) < 0)
	{
		close(tracker_sock);
		return -3;
	}
	
	if(sock_recv_data(&which_storage,sizeof(which_storage),tracker_sock) < 0)
	{
		close(tracker_sock);
		return -4;
	}
	
	close(tracker_sock);
	
	//无服务器
	if(which_storage.ip == 0&&which_storage.port == 0)
		return -5;
	
	char inet_buf[INET_ADDRSTRLEN];
		inet_ntop(AF_INET,&which_storage.ip,inet_buf,INET_ADDRSTRLEN);
	//fflush(stdout);
	if((storage_sock = connect_ip(inet_buf,which_storage.port)) < 0)
	{
		close(storage_sock);
		return -6;
	}

	tcpsetnodelay(storage_sock,3600);
	
	cmd = DOWNLOAD_FROM_STORAGE;
	if(sock_send_data(&cmd,sizeof(cmd),storage_sock) < 0)
	{
		close(storage_sock);
		return -7;
	}
	
	if(sock_send_data(file_name,24,storage_sock) < 0)
	{
		close(storage_sock);
		return -9;
	}
	
	int file_size = 0;
	file_size = string_to_int(file_name + 19,4);
		
	if(sock_recv_file(file_name,file_size,storage_sock) < 0)
	{
		close(storage_sock);
		return -11;
	}
	logDebug("%s",file_name);
	
	close(storage_sock);
	return file_size;
}
int download_file_by_tracker_debug(char *file_name)
{
	int tracker_sock,storage_sock;
	storage_address_info_t which_storage;
	char cmd;
	
	//连接tracker获得最好的storage信息
	logDebug("*connect tracker "i,...);
	if((tracker_sock = connect_ip(g_tracker_info.ip,g_tracker_info.port)) < 0)
	{
		close(tracker_sock);
		return -1;
	}
	logDebug("success!\n",...);

	tcpsetnodelay(tracker_sock,3600);
	
	logDebug("*send DOWNLOAD_GET_STORAGE ",...);
	cmd = DOWNLOAD_GET_STORAGE;
	if(sock_send_data(&cmd,sizeof(cmd),tracker_sock) < 0)
	{
		close(tracker_sock);
		return -2;
	}
	logDebug("success!\n",...);
	
	logDebug("*send group_id and storage_id ",...);
	if(sock_send_data(file_name,24,tracker_sock) < 0)
	{
		close(tracker_sock);
		return -3;
	}
	logDebug("%s,success!\n",file_name);
	
	logDebug("*recv which_storage",...);
	if(sock_recv_data(&which_storage,sizeof(which_storage),tracker_sock) < 0)
	{
		close(tracker_sock);
		return -4;
	}
	logDebug("success!\n",...);
	
	close(tracker_sock);
	
	//无服务器
	logDebug("*check which_storage exists ",...);	
	if(which_storage.ip == 0&&which_storage.port == 0)
		return -5;
	logDebug("success!\n",...);
	
	char inet_buf[INET_ADDRSTRLEN];
	logDebug("connect storage! %s:%d\n",\
		inet_ntop(AF_INET,&which_storage.ip,inet_buf,INET_ADDRSTRLEN),\
			which_storage.port);
	//fflush(stdout);
	if((storage_sock = connect_ip(inet_buf,which_storage.port)) < 0)
	{
		close(storage_sock);
		return -6;
	}
	logDebug("success!\n",...);

	tcpsetnodelay(storage_sock,3600);
	
	logDebug("*send cmd DOWNLOAD_FROM_STORAGE ",...);	
	cmd = DOWNLOAD_FROM_STORAGE;
	if(sock_send_data(&cmd,sizeof(cmd),storage_sock) < 0)
	{
		close(storage_sock);
		return -7;
	}
	logDebug("success!\n",...);
	
	logDebug("*send file_name ",...);
	if(sock_send_data(file_name,24,storage_sock) < 0)
	{
		close(storage_sock);
		return -9;
	}
	logDebug("success!\n",...);
	
	logDebug("*download file ",...);
	int file_size = 0;
	file_size = string_to_int(file_name + 19,4);
	printf("%d\n",file_size);
		
	if((file_size = sock_recv_file(file_name,file_size,storage_sock)) < 0)
	{
		close(storage_sock);
		return -11;
	}
	logDebug("success! %d\n",file_size);
	
	close(storage_sock);
	return file_size;
}

static void get_upload_file_names(char *dir_name)
{
	g_upload_test_info.file_names = malloc(10000*sizeof(char *));

	DIR *p_dir;
	struct dirent *p_dirent;
	struct stat statbuf;

	p_dirent = NULL;
	if((p_dir = opendir(dir_name)) == NULL)
	{
		fprintf(stderr,"---->can\'t open %s\n",dir_name);
		fflush(stderr);
		return ;		
	}
	if(chdir(dir_name) < 0)
		return ;
	while((p_dirent = readdir(p_dir)) != NULL)
	{
		stat(p_dirent->d_name,&statbuf);
		if(S_ISREG(statbuf.st_mode))
		{
			g_upload_test_info.file_names[g_upload_test_info.name_counts] = malloc(sizeof(dir_name)+1+strlen(p_dirent->d_name)+1);
			memcpy(g_upload_test_info.file_names[g_upload_test_info.name_counts],dir_name,sizeof(dir_name));
			strcat(g_upload_test_info.file_names[g_upload_test_info.name_counts],"/");
			strcat(g_upload_test_info.file_names[g_upload_test_info.name_counts],p_dirent->d_name);
			//logDebug("%s",g_upload_test_info.file_names[g_upload_test_info.name_counts]);
			g_upload_test_info.name_counts++;
		}
	}
	if(chdir("..") < 0)
		return ;
	//logDebug("\n");
	closedir(p_dir);
	//logDebug("get_file_names end\n");
	//fflush(stdout);
	
	return ;
}

static void upload_end()
{
	g_upload_test_info.all_sec = time(NULL)-g_upload_test_info.start_time;
	printf("upload_size %lld,upload_sec %d,upload_speed %fM/s\n"
		"return with control+C\n",\
		g_upload_test_info.upload_size,g_upload_test_info.all_sec,\
		g_upload_test_info.upload_size/1048576/(float)g_upload_test_info.all_sec);
	fflush(stdout);

	return ;
}

static void *upload_thread(void *arg)
{
	//logDebug("thread_start\n");
	//fflush(stdout);
	int result;
	if((result = upload_file_by_tracker(arg)) < 0)
	{
		printf("%df%d\n",result,__sync_add_and_fetch(&g_upload_test_info.fail_count,1));
		fflush(stdout);	
		if(__sync_add_and_fetch(&g_upload_test_info.all_count,1) == g_upload_test_info.upload_count)
			upload_end();
		return NULL;
	}
	__sync_fetch_and_add(&g_upload_test_info.upload_size,result);
	printf("s%d\n",__sync_add_and_fetch(&g_upload_test_info.success_count,1));
	fflush(stdout);
	if(__sync_add_and_fetch(&g_upload_test_info.all_count,1) == g_upload_test_info.upload_count)
			upload_end();

	return NULL;
}

int upload_test(char *dir_name,int upload_count,int thread_count)
{
	g_upload_test_info.upload_count = upload_count;
	thread_init(thread_count);
	
	get_upload_file_names(dir_name);
	
	srand((unsigned int)time(NULL));
	
	g_upload_test_info.start_time = time(NULL);
	
	//int i;
	//for(i = 0;i != g_upload_test_info.name_counts;++i)
		//logDebug("%s\n",g_upload_test_info.file_names[i]);
	for(g_upload_test_info.have_upload_count = 0;g_upload_test_info.have_upload_count != upload_count;++g_upload_test_info.have_upload_count)
	{
		//logDebug("add\n");
		//fflush(stdout);
		//logDebug("%s\n",g_upload_test_info.file_names[rand()%g_upload_test_info.name_counts]);
		pool_add_worker(upload_thread,g_upload_test_info.file_names[rand()%g_upload_test_info.name_counts]);
	}

	//logDebug("upload_test_end\n");
	//fflush(stdout);
	
	return 0;	
}

static void get_download_file_names()
{	
	while(fgets(g_download_test_info.file_names[g_download_test_info.name_counts],25,g_uploadlog_fp) != NULL)
	{
		g_download_test_info.file_names[g_download_test_info.name_counts][23] = '\0';
		g_download_test_info.name_counts++;
	}
	return ;
}

static void download_end()
{
	g_download_test_info.all_sec = time(NULL)-g_download_test_info.start_time;
	printf("download_size %lld,download_sec %d,download_speed %fM/s\n"
		"return with control+C\n",\
		g_download_test_info.download_size,g_download_test_info.all_sec,\
		g_download_test_info.download_size/1048576/(float)g_download_test_info.all_sec);
	fflush(stdout);
	
	return ;
}

static void *download_thread(void *arg)
{
	//logDebug("thread_start\n");
	//fflush(stdout);
	int result;
	if((result = download_file_by_tracker(arg)) < 0)
	{
		printf("%df%d\n",result,__sync_add_and_fetch(&g_download_test_info.fail_count,1));
		if(__sync_add_and_fetch(&g_download_test_info.all_count,1) == g_download_test_info.name_counts)
			download_end();
		fflush(stdout);	
		return NULL;
	}
	__sync_fetch_and_add(&g_download_test_info.download_size,result);
	printf("s%d\n",__sync_add_and_fetch(&g_download_test_info.success_count,1));
	fflush(stdout);
	if(__sync_add_and_fetch(&g_download_test_info.all_count,1) == g_download_test_info.name_counts)
		download_end();

	return NULL;
}

int download_test(int thread_count)
{
	thread_init(thread_count);
	
	get_download_file_names();
	
	/*in order not rand*/
	//srand((unsigned int)time(NULL));
	
	g_download_test_info.start_time = time(NULL);

	for(g_download_test_info.have_download_count = 0;g_download_test_info.have_download_count != g_download_test_info.name_counts;++g_download_test_info.have_download_count)
	{
		pool_add_worker(download_thread,g_download_test_info.file_names[g_download_test_info.have_download_count]);
	}
	
	return 0;	
}
