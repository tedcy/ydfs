#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "debug.h"

void init_debug()
{
	struct sigaction act;
#ifdef HAVE_TRACE
	act.sa_sigaction = crit_err_hdlr;
 	act.sa_flags = SA_RESTART | SA_SIGINFO;
 
 	if (sigaction(SIGSEGV, &act, (struct sigaction *)NULL) != 0)
 	{
  		fprintf(stderr, "error setting signal handler for %d (%s)",
    		SIGSEGV, strsignal(SIGSEGV));
 
  		exit(EXIT_FAILURE);
 	}
#endif
	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
 	act.sa_flags = SA_SIGINFO;
	if(sigaction(SIGPIPE, &act, NULL) < 0)
	{
		logInfo("file: "__FILE__", line: %d, " \
			"call sigaction fail, errno: %d, error info: %s", \
			__LINE__, errno, strerror(errno));
  		exit(EXIT_FAILURE);
	}
}

void dead_debug()
{
	*(char *)NULL = 0;
}

void time_debug_start(struct timeval *tv)
{
	gettimeofday(tv,NULL);
}

int time_debug_end(struct timeval *tv)
{
	time_t sec = tv->tv_sec;
	suseconds_t usec = tv->tv_usec;
	gettimeofday(tv,NULL);
	return (tv->tv_sec-sec)*1000000 + tv->tv_usec - usec;
}

#ifdef HAVE_TRACE
void crit_err_hdlr(int sig_num,siginfo_t * info, void * ucontext)
{
 	void*  array[50];
 	void*  caller_address = NULL;
 	char** messages;
 	int    size, i;
 	//sig_ucontext_t*   uc = NULL;
 
 	//uc = (sig_ucontext_t *)ucontext;
 
#ifdef M64
 	// Get the address at the time the signal was raised from the RIP (x86_64)
 	caller_address = (void *) uc->uc_mcontext.rip;   
 	// Get the address at the time the signal was raised from the EIP (x86)
#else 
	#ifdef M32
	caller_address = (void *) uc->uc_mcontext.eip;
	#endif
#endif
 	fprintf(stderr, "signal %d (%s), address is %p from %p\n", \
  		sig_num, strsignal(sig_num), info->si_addr, \
  		(void *)caller_address);
 
 	size = backtrace(array, 50);
 
 	// overwrite sigaction with caller's address
	array[1] = caller_address;
 
 	messages = backtrace_symbols(array, size);
 
 	// skip first stack frame (points here)
for (i = 1; i < size && messages != NULL; ++i)
 	{
  		fprintf(stderr, "[bt]: (%d) %s\n", i, messages[i]);
 	}
 
 	free(messages);
 
 	exit(EXIT_FAILURE);
}
#endif
