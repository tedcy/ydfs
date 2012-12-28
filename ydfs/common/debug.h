#ifndef DEBUG_H
#define DEBUG_H 

#define __USE_POSIX

#include <signal.h>
#ifdef HAVE_TRACE
#include <execinfo.h>
#include <ucontext.h>
#endif

#include <sys/time.h>

/* This structure mirrors the one found in /usr/include/asm/ucontext.h */

/*typedef struct _sig_ucontext {
	unsigned long     uc_flags;
 	struct ucontext   *uc_link;
 	stack_t           uc_stack;
 	struct sigcontext uc_mcontext;
 	sigset_t          uc_sigmask;
}sig_ucontext_t;*/

void init_debug();

void dead_debug();

void time_debug_start(struct timeval *tv);

int time_debug_end(struct timeval *tv);

#ifdef HAVE_TRACE
void crit_err_hdlr(int sig_num, siginfo_t * info, void * ucontext);
#endif

#endif
