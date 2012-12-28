#ifndef SHARE_FUN_H
#define SHARE_FUN_H

#define BUF_MAXSIZE 			65536

#define LOCK_IF_ERROR(functionName,Mutex) \
	do\
	{\
		if(pthread_mutex_lock(&Mutex) != 0)\
		logError(	"file: "__FILE__", line: %d, "\
			functionName" call pthread_mutex_lock fail, "\
			"errno: %d, error info:%s.",\
			__LINE__, errno, strerror(errno));\
	}while(0)
#define UNLOCK_IF_ERROR(functionName,Mutex) \
	do\
	{\
		if(pthread_mutex_unlock(&Mutex) != 0)\
		logError(	"file: "__FILE__", line: %d, "\
			functionName" call pthread_mutex_unlock fail, "\
			"errno: %d, error info:%s.",\
			__LINE__, errno, strerror(errno));\
	}while(0)

#endif
