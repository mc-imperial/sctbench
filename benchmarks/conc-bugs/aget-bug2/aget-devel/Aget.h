
/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 * $Id: Aget.h,v 1.4 2002/12/28 21:30:20 murat Exp $
 *
 */

#ifndef AGET_H
#define AGET_H

#include "Data.h"
#include "Resume.h"

void startHTTP(struct request *);
void startFTP(struct request *);
void resumeDownload(struct hist_data *, int );

#endif
