#ifndef SHARE_NET_H
#define SHARE_NET_H

#include <netinet/in.h>
#include "ae.h"

typedef void file_proc(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
typedef void data_proc(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);

typedef struct NB_FILE_T
{
	int read_size;//recv_file will be recv_size
	int writ_size;//send_file will be send_size
	int file_size;
	off_t start_offlen;
	file_proc *proc;
	int fd;		//for send_file
	/*8bit real_file_name -- 5 bit all_trunk_name(4 bit trunk_name,1 bit storage_id) -- 2bit group_id -- 4 bit start_offlen -- 4 bit file_size*/
	char file_name[25];
	char real_file_name[25];
	char need_name;
}nb_file_t;

typedef struct NB_DATA_T
{
	int need_size;
	int total_size;
	void *buff;
	data_proc* proc;
	data_proc* final_proc;
}nb_data_t;

typedef struct DATA_T
{
	nb_data_t data;
}data_t;

void nb_sock_send_data(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
void nb_sock_recv_data(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);

#endif
