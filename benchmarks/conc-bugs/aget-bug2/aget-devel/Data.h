
/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 * $Id: Data.h,v 1.8 2003/01/27 09:55:55 murat Exp $
 *
 */

#ifndef DATA_H
#define DATA_H

#include <pthread.h>
#include <netinet/in.h>
#include <time.h>


#define	SERVER_TYPE_UNIX	1
#define SERVER_TYPE_NT		2

#include "Defs.h"

typedef struct request {
	char host[MAXHOSTSIZ];		/* Remote host	*/
	char url[MAXURLSIZ];		/* URL		*/
	char file[MAXBUFSIZ];		/* file name	*/
	char lfile[MAXBUFSIZ];		/* if local file name is specified */
	char ip[MAXIPSIZ];		/* Remote IP	*/
	char username[MAXBUFSIZ];
	char password[MAXBUFSIZ];
	int port;
	int ftp_server_type;
	int clength;			/* Content-length	*/
	unsigned char proto;		/* Protocol		*/
	time_t time_taken;		/* Useful when job is resumed */
} request;

typedef struct thread_data {
	struct sockaddr_in sin;
	char getstr[GETREQSIZ];
	long soffset;		/* Start offset		*/
	long foffset;		/* Finish offset	*/
	long offset;		/* Current Offset	*/
	long clength;		/* Content Length	*/
	int fd;
	pthread_t tid;		/* Thread ID		*/
	unsigned char status;	/* thread exit status	*/
	char username[MAXBUFSIZ];	/* Used in ftp_get() */
	char password[MAXBUFSIZ];	/* Used in ftp_get() */
	char url[MAXURLSIZ];		/* Used in ftp_get() */
	int head_sd;			/* Used for reusing head connection	*/
} thread_data;

#endif
