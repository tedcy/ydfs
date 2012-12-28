#include <unistd.h>
#include <string.h>
#include <netinet/tcp.h> //定义了setsockopt函数的参数
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/sendfile.h>

#include <sys/ioctl.h>		//这两个头文件获得本地IP地址所用
#include <linux/if.h>    	//定义了struct ifreq结构

#include "sockopt.h"
#include "debug.h"

/*
 * this codes out of service
data_t* init_data()
{
	data_t *pData;
	
	pData = malloc(sizeof(*pData));
	memset(&pData->data,0,sizeof(pData->data));
	
	return pData;
}

void clean_data(data_t *pData)
{
	if(pData == NULL)
		return ;
	free(pData);
}

void refresh_data(data_t *pData)
{
    	if(pData == NULL)
		return ;
	memset(&pData->data,0,sizeof(pData->data));
	return ;
}*/

int sock_recv_file(const char* file_name,int file_size,const int sockfd)
{
	size_t recv_count;
	size_t now_recv_count;
	FILE *fp;
	char READ_FILE_BUF[BUF_MAXSIZE];
	
	if((fp = fopen(file_name,"w")) == NULL)
	{
		logError(	"file: "__FILE__",line: %d,"\
			"sock_recv_file call open file failed,"\
			"errno: %d,error info: %s",\
			__LINE__,errno,strerror(errno));
		fclose(fp);
		return -1;
	}

	recv_count = 0;
	while(recv_count+BUF_MAXSIZE <= file_size)
	{
		if((now_recv_count = read(sockfd,READ_FILE_BUF,BUF_MAXSIZE)) == -1)
		{
			logError(	"file: "__FILE__",line :%d, "\
				"sock_recv_file read sock failed,"\
				"errno: %d,error info: %s",\
				__LINE__,errno,strerror(errno));
			fclose(fp);
			return -2;
		}
		fwrite(READ_FILE_BUF,1,now_recv_count,fp);
		recv_count += now_recv_count;
	}
	while(recv_count != file_size)
	{
		if((now_recv_count = read(sockfd,READ_FILE_BUF,file_size-recv_count)) == -1)
		{
			logError(	"file: "__FILE__",line :%d, "\
				"sock_recv_file read sock failed,"\
				"errno: %d,error info: %s",\
				__LINE__,errno,strerror(errno));
			fclose(fp);
			return -3;
		}
		fwrite(READ_FILE_BUF,1,now_recv_count,fp);
		recv_count += now_recv_count;
	}
	fclose(fp);
	return recv_count;
}

int sock_send_file(const char* file_name,const int sockfd)
{
	struct stat stat_buff;
	off_t offset;
	int fd;
	int size;

	offset = 0;
	stat(file_name,&stat_buff);

	size = stat_buff.st_size;
	if(sock_send_data(&size,sizeof(int),sockfd) < 0)
		return -2;
	
	if((fd = open(file_name,O_RDONLY)) == -1)
	{
		logError(	"file: "__FILE__",line: %d,"\
			"sock_send_file call open file %s failed,"\
			"errno: %d,error info: %s",\
			__LINE__,file_name,errno,strerror(errno));
		return -1;
	}
	while(offset != stat_buff.st_size)
	{
		if(sendfile(sockfd,fd,&offset,stat_buff.st_size-offset) == -1)
		{
			logError(	"file: "__FILE__",line :%d, "\
				"sock_send_file sendfile failed,"\
				"errno: %d,error info: %s",\
				__LINE__,errno,strerror(errno));
			close(fd);
			return -2;
		}
	}
	close(fd);
	return offset;
}

int old_sock_send_file(const char* file_name,const int sockfd)
{
	size_t send_count,file_size;
	size_t now_send_count;
	FILE *fp;
	char READ_FILE_BUF[BUF_MAXSIZE];
	
	if((fp = fopen(file_name,"r")) == NULL)
	{
		logError(	"file: "__FILE__",line: %d,"\
			"old_sock_send_file call open file %s failed,"\
			"errno: %d,error info: %s",\
			__LINE__,file_name,errno,strerror(errno));
		return -1;
	}
	fseek(fp,0,SEEK_END);
	file_size = ftell(fp);
	fseek(fp,0,SEEK_SET);
	
	if(sock_send_data(&file_size,sizeof(file_size),sockfd) < 0)
		return -2;

	send_count = 0;
	while(send_count != file_size)
	{
		now_send_count = fread(READ_FILE_BUF,1,BUF_MAXSIZE,fp);
		if(write(sockfd,READ_FILE_BUF,now_send_count) == -1)
		{
			logError(	"file: "__FILE__",line :%d, "\
				"sock_send_file write sock failed,"\
				"errno: %d,error info: %s",\
				__LINE__,errno,strerror(errno));
			return -2;
		}
		send_count += now_send_count;
	}
	fclose(fp);
	return file_size;
}

int sock_send_data(void *buff,size_t size,const int sockfd)
{
	size_t send_size = 0;
	size_t total_size = 0;
	while(total_size != size)
	{
		if((send_size = write(sockfd,buff,size)) == -1)
		{
			logError(	"file: "__FILE__",line :%d, "\
				"sock_send_data write sock failed,"\
				"errno: %d,error info: %s",\
				__LINE__,errno,strerror(errno));
			close(sockfd);
			return -1;
		}
		total_size += send_size;
	}
	return total_size;
}

int sock_recv_data(void *buff,size_t size,const int sockfd)
{
	size_t recv_size = 0;
	size_t total_size = 0;
	
	while(total_size != size)
	{
		if((recv_size = read(sockfd,buff,size)) == -1)
		{
			logError(	"file: "__FILE__",line :%d, "\
				"sock_recv_data read sock failed,"\
				"errno: %d,error info: %s",\
				__LINE__,errno,strerror(errno));
			close(sockfd);
			return -1;
		}
		total_size += recv_size;
	}
	return total_size;
}

int get_my_ip(int sockfd,in_addr_t *target_ip)
{
	int i;
	char buf[512];
	struct ifconf ifconf;
	struct ifreq *ifreq;
	char *ip;
	//初始化ifconf
	ifconf.ifc_len = 512;
	ifconf.ifc_buf = buf;
	if(ioctl(sockfd,SIOCGIFCONF,&ifconf) < 0)
	{
		logError(	"file: "__FILE__",line: %d,"\
			"get_my_ip call ioctl failed,"\
			"errno: %d,error info: %s",\
			__LINE__,errno,strerror(errno));
		return -1;
	}
	ifreq = (struct ifreq*)buf;

	for(i = ifconf.ifc_len/sizeof(struct ifreq);i != -1;--i)
	{
		ip = inet_ntoa(((struct sockaddr_in*) &(ifreq->ifr_addr))->sin_addr);
		if(strcmp(ip,"127.0.0.1") == 0)
		{
			++ifreq;
			continue;
		}
		*target_ip = ((struct sockaddr_in*) &(ifreq->ifr_addr))->sin_addr.s_addr;
		//logError("%s",ip);
		return 0;
	}
	return -2;
}

int get_ip(getnamefunc* getname,int sockfd,in_addr_t *ip)
{
	struct sockaddr_in addr;
	socklen_t sin_size;
	
	sin_size = sizeof(addr);
	
	if(getname(sockfd,(struct sockaddr*)&addr,&sin_size) < 0)
	{
		logError(	"file: "__FILE__",line: %d,"\
			"get_ip call get_ip failed,"\
			"errno: %d,error info: %s",\
			__LINE__,errno,strerror(errno));
		return -1;
	}
	*ip = addr.sin_addr.s_addr;
	return 0;
}

int connect_ip(const char* ip,const unsigned short port)
{
	int sockfd;
	int result;
	struct sockaddr_in addr;

	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		logError(	"file: "__FILE__",line :%d"\
			"connect_ip call socket failed,"\
			"errno :%d,errno info: %s",\
			__LINE__,errno,strerror(errno));
		return -1;
	}

	result = 1;
	if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&result,sizeof(int)) < 0)
	{
		logError(	"file :"__FILE__",line :%d"\
			"call setsockopt failed,"\
			"errno: %d,error info: %s",\
			__LINE__,errno,strerror(errno));
		return -2;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if(inet_pton(AF_INET,ip,&addr.sin_addr) != 1)
	{
		logError(	"file: "__FILE__",line :%d"\
			"connect_ip call inet_pton failed,"\
			"errno :%d,errno info: %s",\
			__LINE__,errno,strerror(errno));
		return -3;
	}

	do
	{
		if(connect(sockfd,(struct sockaddr*)&addr,sizeof(addr)) < 0)
		{
			if(errno == EAGAIN)
				continue ;
			logInfo(	"file :"__FILE__",line :%d"\
				"connect_ip %s:%d call connect failed,"\
				"errno :%d,errno info: %s",\
				__LINE__,ip,port,errno,strerror(errno));
			return -4;
		}
		else
			break;
	}while(1);
	return sockfd;
}

int sock_server(const int port)
{
	int sock,result;
	struct sockaddr_in bindaddr;
	
	if((sock = socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		logError(	"file :"__FILE__",line :%d"\
			"socket create failed,"\
			"errno :%d,error info: %s",\
			__LINE__,errno,strerror(errno));
		return -1;
	}
	bindaddr.sin_family = AF_INET;
	bindaddr.sin_port = htons(port);
	bindaddr.sin_addr.s_addr = INADDR_ANY;
	
	result = 1;
	if((result = setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&result,sizeof(int))) < 0)
	{
		logError(	"file :"__FILE__",line :%d"\
			"call setsockopt failed,"\
			"errno: %d,error info: %s",\
			__LINE__,errno,strerror(errno));
		return -2;
	}

	if((result = bind(sock,(struct sockaddr*)&bindaddr,sizeof(bindaddr))) < 0)
	{
		logError(	"file :"__FILE__",line :%d"
			"bind failed"\
			"error :%d,error info :%s",\
			__LINE__,errno,strerror(errno));
		return -3;
	}
	if((result = listen(sock,BACKLOG)) < 0)
	{
		logError(	"file :"__FILE__",line :%d"\
			"listen failed"\
			"error :%d,error info: %s",\
			__LINE__,errno,strerror(errno));
		return -4;
	}
	if(setnonblocking(sock) < 0)
	{
		logError(	"file :"__FILE__",line :%d"\
			"listen sock set nonblocking failed.",\
			"error :%d,error info: %s",\
			__LINE__,errno,strerror(errno));
		return -5;
	}
	return sock;
}

int setnonblocking(int sock)
{
	int opts;
	opts=fcntl(sock,F_GETFL);
	if(opts<0)
	{
		perror("fcntl(sock,GETFL)");
		return -1;
	}
	opts = opts|O_NONBLOCK;
	if(fcntl(sock,F_SETFL,opts)<0)
	{
		perror("fcntl(sock,SETFL,opts)");
		return -2;
	}
	return 0;
}

int tcpsetserveropt(int fd, const int timeout)
{
	int flags;
	int result;

	struct linger linger;
	struct timeval waittime;

	linger.l_onoff = 1;
	linger.l_linger = timeout;
	if (setsockopt(fd, SOL_SOCKET, SO_LINGER, \
                &linger, (socklen_t)sizeof(struct linger)) < 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"setsockopt failed, errno: %d, error info: %s", \
			__LINE__, errno, strerror(errno));
		return errno != 0 ? errno : ENOMEM;
	}

	waittime.tv_sec = timeout;
	waittime.tv_usec = 0;

	if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO,
               &waittime, (socklen_t)sizeof(struct timeval)) < 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"setsockopt failed, errno: %d, error info: %s", \
			__LINE__, errno, strerror(errno));
		return errno != 0 ? errno : ENOMEM;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
               &waittime, (socklen_t)sizeof(struct timeval)) < 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"setsockopt failed, errno: %d, error info: %s", \
			__LINE__, errno, strerror(errno));
		return errno != 0 ? errno : ENOMEM;
	}
	
	flags = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, \
		(char *)&flags, sizeof(flags)) < 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"setsockopt failed, errno: %d, error info: %s", \
			__LINE__, errno, strerror(errno));
		return errno != 0 ? errno : EINVAL;
	}

	if ((result=tcpsetkeepalive(fd, 2 * timeout + 1)) != 0)
	{
		return result;
	}

	return 0;
}

int tcpsetkeepalive(int fd, const int idleSeconds)
{
	int keepAlive;
	int keepIdle;
	int keepInterval;
	int keepCount;

	keepAlive = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, \
		(char *)&keepAlive, sizeof(keepAlive)) < 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"setsockopt failed, errno: %d, error info: %s", \
			__LINE__, errno, strerror(errno));
		return errno != 0 ? errno : EINVAL;
	}

	keepIdle = idleSeconds;
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, (char *)&keepIdle, \
		sizeof(keepIdle)) < 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"setsockopt failed, errno: %d, error info: %s", \
			__LINE__, errno, strerror(errno));
		return errno != 0 ? errno : EINVAL;
	}

	keepInterval = 75;
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, (char *)&keepInterval, \
		sizeof(keepInterval)) < 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"setsockopt failed, errno: %d, error info: %s", \
			__LINE__, errno, strerror(errno));
		return errno != 0 ? errno : EINVAL;
	}

	keepCount = 3;
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, (char *)&keepCount, \
		sizeof(keepCount)) < 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"setsockopt failed, errno: %d, error info: %s", \
			__LINE__, errno, strerror(errno));
		return errno != 0 ? errno : EINVAL;
	}

	return 0;
}

int tcpprintkeepalive(int fd)
{
	int keepAlive;
	int keepIdle;
	int keepInterval;
	int keepCount;
	socklen_t len;

	len = sizeof(keepAlive);
	if (getsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, \
		(char *)&keepAlive, &len) < 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"setsockopt failed, errno: %d, error info: %s", \
			__LINE__, errno, strerror(errno));
		return errno != 0 ? errno : EINVAL;
	}

	len = sizeof(keepIdle);
	if (getsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, (char *)&keepIdle, \
		&len) < 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"setsockopt failed, errno: %d, error info: %s", \
			__LINE__, errno, strerror(errno));
		return errno != 0 ? errno : EINVAL;
	}

	len = sizeof(keepInterval);
	if (getsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, (char *)&keepInterval, \
		&len) < 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"setsockopt failed, errno: %d, error info: %s", \
			__LINE__, errno, strerror(errno));
		return errno != 0 ? errno : EINVAL;
	}

	len = sizeof(keepCount);
	if (getsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, (char *)&keepCount, \
		&len) < 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"setsockopt failed, errno: %d, error info: %s", \
			__LINE__, errno, strerror(errno));
		return errno != 0 ? errno : EINVAL;
	}

	logError("keepAlive=%d, keepIdle=%d, keepInterval=%d, keepCount=%d", 
		keepAlive, keepIdle, keepInterval, keepCount);

	return 0;
}

int tcpsetnodelay(int fd, const int timeout)
{
	int flags;
	int result;

	if ((result=tcpsetkeepalive(fd, 2 * timeout + 1)) != 0)
	{
		return result;
	}

	flags = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, \
			(char *)&flags, sizeof(flags)) < 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"setsockopt failed, errno: %d, error info: %s", \
			__LINE__, errno, strerror(errno));
		return errno != 0 ? errno : EINVAL;
	}

	return 0;
}
