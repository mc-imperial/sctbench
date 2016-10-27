
/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 * $Id: Download.h,v 1.3 2003/01/16 20:15:23 rao Exp $
 *
 */

#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include <pthread.h>

void *http_get(void *);

#endif
