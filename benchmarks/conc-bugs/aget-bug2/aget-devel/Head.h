
/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 * $Id: Head.h,v 1.4 2003/01/27 09:55:56 murat Exp $
 *
 */

#ifndef HEAD_H
#define HEAD_H

#include "Data.h"

void http_head_req(struct request *);
int ftp_head_req(struct request *, int *);

#endif
