#include "log.h"
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#define fileno(__P) ((__P)->_fileno)

static void doLog(const char* prefix, const char* text, const char *caption);

void logInfo(const char* format, ...)
{
	char logBuffer[LINE_MAX];	
	va_list ap;
	va_start(ap, format);
	vsnprintf(logBuffer, sizeof(logBuffer), format, ap);    
	doLog(g_error_file_prefix, logBuffer,"INFO ");
	va_end(ap);
}

#ifdef __DEBUG__
void logDebug(const char* format, ...)
{
	char logBuffer[LINE_MAX];
	va_list ap;
	va_start(ap, format);
	vsnprintf(logBuffer, sizeof(logBuffer), format, ap);    
	doLog(g_error_file_prefix, logBuffer,"DEBUG");
	va_end(ap);
}
#endif

void logError(const char* format, ...)
{
	char logBuffer[LINE_MAX];	
	va_list ap;
	va_start(ap, format);
	vsnprintf(logBuffer, sizeof(logBuffer), format, ap);    
	doLog(g_error_file_prefix, logBuffer,"ERROR");
	va_end(ap);
}

void logConfig(const char* format, ...)
{
	char logBuffer[LINE_MAX];	
	va_list ap;
	va_start(ap, format);
	vsnprintf(logBuffer, sizeof(logBuffer), format, ap);    
	doLog(g_error_file_prefix, logBuffer,"CONFIG");
	va_end(ap);
}

static void doLog(const char* prefix, const char* text, const char* caption)
{
	time_t t;
	struct tm *pCurrentTime;
	char dateBuffer[32];
	FILE *fp;
	int fd;
	struct flock lock;

	t = time(NULL);
	pCurrentTime = localtime(&t);
	strftime(dateBuffer, sizeof(dateBuffer), "[%Y-%m-%d %X]", pCurrentTime);
		
	if((fp = fopen("log","a")) == NULL)
	{
		fp = stderr;
	}
	
	fd = fileno(fp);
	
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_CUR;
	lock.l_start = 0;
	lock.l_len = 1000;
	if (fcntl(fd, F_SETLKW, &lock) == 0)
	{
		fprintf(fp, "%s - %s %s\n", caption, dateBuffer, text);
	}
	
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLKW, &lock);
	if (fp != stderr)
	{
		fclose(fp);
	}
	return ;
}

void write_file_log(const char *str,int len,FILE *fp)
{
	int fd;
	struct flock lock;
	fd = fileno(fp);
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_CUR;
	lock.l_start = 0;
	lock.l_len = len;
	if (fcntl(fd, F_SETLKW, &lock) == 0)
	{
		fwrite(str,1,len,fp);
	}
	fflush(fp);
	
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLKW, &lock);

	return ;
}
