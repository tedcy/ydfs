#ifndef STORAGE_SERVICE_H
#define STORAGE_SERVICE_H

#include <stdio.h>

struct aeEventLoop;

void nb_sock_send_file(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
int heart_beat_init(struct aeEventLoop *eventLoop, long long id, void *clientData);
int check_if_sync_need_start(struct aeEventLoop *eventLoop, long long id, void *clientData);
void do_upload_to_storage(struct aeEventLoop *eventLoop, int incomesock, void *clientData, int mask);
void accept_command(struct aeEventLoop *eventLoop, int incomesock, void *clientData, int mask);

#endif
