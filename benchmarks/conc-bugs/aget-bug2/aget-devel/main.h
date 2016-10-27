
/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 * $Id: main.h,v 1.3 2002/12/28 21:30:21 murat Exp $
 *
 */

#ifndef MAIN_H
#define MAIN_H

#include <pthread.h>
#include "Data.h"

char *fullurl;

int nthreads;
int fsuggested = 0;

struct request *req;		/* Download jobs		*/
pthread_t hthread;		/* Helper thread for signals	*/
struct thread_data *wthread;	/* Worker Threads		*/

#endif
