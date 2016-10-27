
/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 * $Id: Ftp.h,v 1.3 2003/01/16 20:15:23 rao Exp $
 *
 */

#ifndef FTP_H
#define FTP_H

#include <pthread.h>

void *ftp_get(void *);

#endif
