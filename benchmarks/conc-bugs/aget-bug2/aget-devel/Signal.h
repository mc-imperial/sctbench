/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 * $Id: Signal.h,v 1.3 2003/01/26 14:50:15 murat Exp $
 *
 */

#ifndef SIGNAL_H
#define SIGNAL_H

#include <signal.h>
#include <pthread.h>

void *signal_waiter(void *);
void sigint_handler(void);
void sigalrm_handler(void);

#endif
