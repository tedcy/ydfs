#ifndef SOCKOPT_H
#define SOCKOPT_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "log.h"
#include "share_fun.h"

#define BACKLOG 			511


typedef int getnamefunc(int socket, struct sockaddr *address, socklen_t *address_len);

#define get_peer_ip(sock,ip) get_ip(getpeername,sock,ip)
#define get_sock_ip(sock,ip) get_ip(getsockname,sock,ip)

int sock_send_file(const char *filename,const int sockfd);
int old_sock_send_file(const char *filename,const int sockfd);
int sock_recv_file(const char *filename,int file_size,const int sockfd);
int sock_send_data(void *buff,size_t size,const int sockfd);
int sock_recv_data(void *buff,size_t size,const int sockfd);
int get_my_ip(int sockfd,in_addr_t *target_ip);
int get_ip(getnamefunc* getname,int sockfd,in_addr_t *ip);
int connect_ip(const char* ip,const unsigned short port);
int sock_server(const int port);
int setnonblocking(int sock);
int tcpsetserveropt(int fd, const int timeout);
int tcpsetkeepalive(int fd, const int idleSeconds);
int tcpprintkeepalive(int fd);
int tcpsetnodelay(int fd, const int timeout);
#endif
