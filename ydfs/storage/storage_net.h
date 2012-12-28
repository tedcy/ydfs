#ifndef STORAGE_NET_H
#define STORAGE_NET_H

#include "sockopt.h"
#include "share_net.h"
#include "storage_global.h"

storage_client_t* init_storage_client(nio_node_t *nio_node);
void clean_storage_client(storage_client_t *pClient);
void refresh_storage_client_data(storage_client_t *pClient);
void refresh_storage_client_file(storage_client_t *pClient);
void finish_storage_CLOSE_WAIT_stat(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
void new_nb_sock_send_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
void nb_sock_send_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
void nb_sock_recv_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
int init_storage_service();
int create_storage_service();
int run_storage_service();
void accept_tcp(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
void one_work_start(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);

#define storage_nio_read(nio_data,nio_data_size,nio_proc_name,nio_final_proc_name,nio_function_name)\
	do\
	{\
		pClient->data.buff = nio_data;\
		pClient->data.need_size = nio_data_size;\
		pClient->data.total_size = 0;\
		pClient->data.proc = nio_proc_name;\
		pClient->data.final_proc = nio_final_proc_name;\
	\
		if(aeCreateFileEvent(eventLoop,sockfd, AE_READABLE,nb_sock_recv_data,pClient) != AE_OK)\
		{\
			logError(	"file :"__FILE__",line :%d"\
				nio_function_name" CreateFileEvent failed.",\
				__LINE__);\
			exit(1) ;\
		}\
		break ;\
	}while(0)

#define storage_nio_write(nio_data,nio_data_size,nio_proc_name,nio_final_proc_name,nio_function_name)\
	do\
	{\
		pClient->data.buff = nio_data;\
		pClient->data.need_size = nio_data_size;\
		pClient->data.total_size = 0;\
		pClient->data.proc = nio_proc_name;\
		pClient->data.final_proc = nio_final_proc_name;\
	\
		if(aeCreateFileEvent(eventLoop,sockfd, AE_WRITABLE,nb_sock_send_data,pClient) != AE_OK)\
		{\
			logError(	"file :"__FILE__",line :%d"\
				nio_function_name" CreateFileEvent failed.",\
				__LINE__);\
			exit(1) ;\
		}\
		break ;\
	}while(0)

#endif
