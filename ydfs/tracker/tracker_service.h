#ifndef THRACKER_SERVICE_H
#define THRACKER_SERVICE_H

#include <stdio.h>
//#include "filename.h"
//#include "tracker_proto.h"
//#include "storage_proto.h"
//#include "storage_sync.h"

struct aeEventLoop;

int check_heart_beat(struct aeEventLoop *eventLoop, long long id, void *clientData);
int tracker_start_sync(struct aeEventLoop *eventLoop, long long id, void *clientData);
void heart_beat_stop(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);
int check_storage_pool(struct aeEventLoop *eventLoop,long long id,void *clientData);
void accept_command(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);

#endif
