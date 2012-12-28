#include "share_net.h"
#include "sockopt.h"

void nb_sock_send_data(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	nb_data_t *data;
	int send_size;
	
	data = &((data_t *)clientData)->data;
	
	if((send_size = write(sockfd,(char*)(data->buff)+data->total_size,data->need_size-data->total_size)) == -1&&errno != EAGAIN)
	{
		if(errno != EAGAIN)
		{
			logError(	"file: "__FILE__",line :%d, "\
					"nb_sock_send_data write sock failed,"\
					"errno: %d,error info: %s",\
					__LINE__,errno,strerror(errno));
			aeDeleteFileEvent(eventLoop,sockfd,AE_WRITABLE);
			if(data->final_proc != NULL)
				data->final_proc(eventLoop,sockfd,clientData,AE_WRITABLE);
		}
		return ;
	}
	/*no need to do this*/
	if(send_size == 0)
	{
		logInfo(	"file: "__FILE__",line :%d, "\
				"nb_sock_send_data write sock failed",\
				__LINE__);
		aeDeleteFileEvent(eventLoop,sockfd,AE_WRITABLE);
		if(data->final_proc != NULL)
			data->final_proc(eventLoop,sockfd,clientData,AE_WRITABLE);
		return ;
	}
	data->total_size+=send_size;
	if(data->need_size != data->total_size)
		return ;
	aeDeleteFileEvent(eventLoop,sockfd,AE_WRITABLE);
	if(data->proc != NULL)
		data->proc(eventLoop,sockfd,clientData,AE_WRITABLE);
	return ;
}

void nb_sock_recv_data(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask)
{
	nb_data_t *data;
	int recv_size;
	
	data = &((data_t *)clientData)->data;
	
	if((recv_size = read(sockfd,(char*)(data->buff)+data->total_size,data->need_size-data->total_size)) == -1)
	{
		if(errno != EAGAIN)
		{
			logError(	"file: "__FILE__",line :%d, "\
					"nb_sock_recv_data recv sock failed,"\
					"errno: %d,error info: %s",\
					__LINE__,errno,strerror(errno));
			aeDeleteFileEvent(eventLoop,sockfd,AE_READABLE);
			if(data->final_proc != NULL)
				data->final_proc(eventLoop,sockfd,clientData,AE_READABLE);	
		}
		return ;
	}
	if(recv_size == 0)
	{
		logInfo(	"file: "__FILE__",line :%d, "\
				"nb_sock_recv_data recv sock failed",\
				__LINE__);
		aeDeleteFileEvent(eventLoop,sockfd,AE_READABLE);
		if(data->final_proc != NULL)
			data->final_proc(eventLoop,sockfd,clientData,AE_READABLE);
		return;
	}
	data->total_size+=recv_size;
	if(data->need_size != data->total_size)
		return;
	aeDeleteFileEvent(eventLoop,sockfd,AE_READABLE);
	if(data->proc != NULL)	
		data->proc(eventLoop,sockfd,clientData,AE_READABLE);
	return;
}
