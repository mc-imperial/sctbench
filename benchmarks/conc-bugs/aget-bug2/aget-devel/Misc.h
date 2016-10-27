
/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 * $Id: Misc.h,v 1.8 2003/01/27 07:30:49 murat Exp $
 *
 */

#ifndef MISC_H
#define MISC_H

#include "Data.h"

#define	LOGSIZ	1024

int calc_offset(int, int, int);
int numofthreads(int);
void parse_url(char *, struct request *);
void usage();
void revstr(char *);				/* Reverse String				*/
void Log(char *, ...);				/* Log 						*/
void updateProgressBar();
void handleHttpRetcode(char *);
void sendCmd(int, char *, int, char *, int);
void recvRep(int, char *, int, int);
int recv_data(int, char *, int);
int process(char *, char *, int *, int *, int *, int *, int);
void handleFTPRetcode(char *rbuf);
int recv_reply(int, char *, int, int);

#endif

