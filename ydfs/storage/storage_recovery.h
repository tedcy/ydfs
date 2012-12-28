#ifndef STORAGE_RECOVERY_H
#define STORAGE_RECOVERY_H

#include "storage_sync.h"

void do_recovery_from_storage(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);

void start_check_recovery_count_stamp(struct aeEventLoop *eventLoop, int sockfd, void *clientData, int mask);

#endif
