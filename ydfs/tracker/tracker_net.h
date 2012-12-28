#ifndef TRACKER_NET_H
#define TRACKER_NET_H

#include "sockopt.h"
#include "share_net.h"
#include "tracker_global.h"

tracker_client_t* init_tracker_client(nio_node_t *nio_node);
void clean_tracker_client(tracker_client_t *pClient);
void refresh_tracker_client_data(tracker_client_t *pClient);
void finish_tracker_CLOSE_WAIT_stat(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
void accept_tcp(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
int init_tracker_service();
int create_tracker_service();
int run_tracker_service();

#define tracker_nio_read(nio_data,nio_data_size,nio_proc_name,nio_final_proc_name,nio_function_name)\
	do\
	{\
		pClient->data.buff = nio_data;\
		pClient->data.need_size = nio_data_size;\
		pClient->data.total_size = 0;\
		pClient->data.proc = nio_proc_name;\
		pClient->data.final_proc = nio_final_proc_name;\
	\
		if(aeCreateFileEvent(eventLoop, sockfd, AE_READABLE,nb_sock_recv_data,pClient) != AE_OK)\
		{\
			logError(	"file :"__FILE__",line :%d"\
				nio_function_name" CreateFileEvent failed.",\
				__LINE__);\
			exit(1) ;\
		}\
		break ;\
	}while(0)

#define tracker_nio_write(nio_data,nio_data_size,nio_proc_name,nio_final_proc_name,nio_function_name)\
	do\
	{\
		pClient->data.buff = nio_data;\
		pClient->data.need_size = nio_data_size;\
		pClient->data.total_size = 0;\
		pClient->data.proc = nio_proc_name;\
		pClient->data.final_proc = nio_final_proc_name;\
	\
		if(aeCreateFileEvent(eventLoop, sockfd, AE_WRITABLE,nb_sock_send_data,pClient) != AE_OK)\
		{\
			logError(	"file :"__FILE__",line :%d"\
				nio_function_name" CreateFileEvent failed.",\
				__LINE__);\
			exit(1) ;\
		}\
		break ;\
	}while(0)

#endif
