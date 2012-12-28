#ifndef LOG_H
#define LOG_H

#ifndef LINE_MAX
#define LINE_MAX 2048 
#endif

#include <errno.h>
#include <stdio.h>

char g_error_file_prefix[64];

void logInfo(const char* format, ...);
#ifdef __DEBUG__
void logDebug(const char* format, ...);
#else
#define logDebug(format,...)
#endif
void logError(const char* format, ...);
void logConfig(const char* format, ...);

void write_file_log(const char *str,int len,FILE *fp);
#endif
