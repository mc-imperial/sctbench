/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 * $Id: Defs.h,v 1.12 2004/05/31 12:33:11 murat Exp $
 *
 */

#ifndef DEFS_H
#define DEFS_H

enum {
	GETREQSIZ = 256,
	GETRECVSIZ = 8192,
	MAXBUFSIZ = 8192,
	HEADREQSIZ = 512,
	MAXURLSIZ = 1024,
	MAXHOSTSIZ = 1024,
	MAXIPSIZ = 16,
	MAXTHREADS = 10,
	VALSIZE = 256,
	HTTPPORT = 80,
	UNKNOWNREQ = 2,
	FTPREQ = 21,
	PROTO_HTTP = 0xFF,
	PROTO_FTP = 0x00,
	STAT_OK = 0xFF,		/* Download completed successfully	*/
	STAT_INT = 0x0F,	/* ^C caught, download interrupted	*/
	STAT_ERR = 0x00		/* Download finished with error		*/
};


#define	PROGVERSION  "EnderUNIX Aget 0.59"
#define	HEADREQ  "HEAD %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\nConnection: close\r\n\r\n"
#define	GETREQ  "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\nRange: bytes=%ld-\r\nConnection: close\r\n\r\n"

#endif
