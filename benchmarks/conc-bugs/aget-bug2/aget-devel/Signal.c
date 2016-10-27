/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 * $Id: Signal.c,v 1.5 2004/04/18 19:42:00 murat Exp $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

#include "Signal.h"
#include "Data.h"
#include "Resume.h"
#include "Misc.h"

extern int nthreads;
extern struct thread_data *wthread;
extern struct request *req;
extern int bwritten;
extern pthread_mutex_t bwritten_mutex;
extern pthread_t main_tid;


void *signal_waiter(void *arg)
{
	int signal;
	sigset_t set;

	arg = NULL;
	sigint_handler();
	return (void*)0;
	/* Set Cancellation Type to Asynchronous */
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	sigfillset(&set);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGALRM);
	/* Set off alarm so that progressbar is updated at regular intervals	*/
	alarm(1);	

	while(1) {
		/* TODO for Solaris, fix bugs here. */
		#ifdef SOLARIS
		sigwait(&set);
		#else
		sigwait(&set, &signal);
		#endif
		switch(signal) {
			case SIGINT:
				sigint_handler();
				break;
			case SIGALRM:
				sigalrm_handler();
				break;
		}
	}
}

void sigint_handler(void)
{
	int i;

	fflush(stdout);
	printf("^C caught, terminating download job. Please wait...\n");

	/* Cancel main thread - can cause serious problems otherwise.
	 * This is because, we will be restarting threads which are interrupted
	 * or which terminate in error in Aget.c
	 */
	//pthread_cancel(main_tid);
	//pthread_join(main_tid, NULL);

	//for (i = 0; i < nthreads; i++) {
	//	pthread_cancel(wthread[i].tid);
	//	wthread[i].status &= STAT_INT;		/* Interrupted download	*/
	//}
	
	printf("Download job terminated. Now saving download job...\n");
	save_log();

	exit(0);
}

void sigalrm_handler(void)
{
	updateProgressBar();
	alarm(1);
}
