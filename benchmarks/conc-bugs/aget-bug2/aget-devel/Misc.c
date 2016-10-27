
/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 * $Id: Misc.c,v 1.23 2004/06/01 06:57:02 murat Exp $
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "Misc.h"
#include "Data.h"

#define VALSIZE	256

extern unsigned int bwritten;	/* Used by updateProgressBar() */
extern request *req;

extern char ftpanonymoususer[VALSIZE];
extern char ftpanonymouspass[VALSIZE];
extern int preferredthread;
extern char http_proxyhost[VALSIZE];
extern char http_proxyuser[VALSIZE];
extern char http_proxypass[VALSIZE];



void parse_url(char *url, struct request *req)
{
	char *s;
	int i, j, k;

	i = j = k = 0;
	s = url;
	if ((strncmp(url, "https://", 8)) == 0) {
		fprintf(stderr, "Error: Currently Aget doesn't support HTTPS requests...\n");
		exit(1);
	} else if ((strncmp(url, "ftp://", 6)) == 0) {
		req->port = (req->port == 0 ? 21 : req->port);
		req->proto = PROTO_FTP;
	} else if ((strncmp(url, "http://", 7)) == 0) {
		req->port = (req->port == 0 ? 80 : req->port);
		req->proto = PROTO_HTTP;
	} else {
		fprintf(stderr, "Error: No protocol specified (http:// or ftp://)...\n");
		exit(1);
	}

	if ((strncmp(url, "http://", 7)) != 0 && req->proto == PROTO_HTTP) {
		fprintf(stderr, "Error: URL should be of the form http://...\n");
		exit(1);
	}

	if ((strncmp(url, "ftp://", 6)) != 0 && req->proto == PROTO_FTP) {
		fprintf(stderr, "Error: URL should be of the form ftp://...\n");
		exit(1);
	}

	s = (req->proto == PROTO_HTTP ? url + 7 : url + 6); 	/* Jump pass http://  or ftp:// part depending on the protocol */
	for (i = 0; *s != '/'; i++, s++) {
		if (i > MAXHOSTSIZ) {
			fprintf(stderr, "Error: Cannot get hostname from URL...\n");
			exit(1);
		}
		if (*s == ':') {	/* If username:password is supplied: http://naveen:12345@x.y.com/file */
			while(*s != '/') {
				req->username[j++] = *--s;
				i--;
			}
			req->username[--j] = '\0';
			revstr(req->username);

			while(*s != ':') s++;	/* Skip past user name --Naveen */

			while(1) {
				if (*s == ':') {
					while(*s != '@') {
						if (k > MAXBUFSIZ) {
							fprintf(stderr, "Error: Cannot get password from URL...\n");
							exit(1);
						}
						req->password[k++] = *++s;
					}
					break;
				}
				s++;
			}
			req->password[--k] = '\0';
		}
		req->host[i] = *s;
	}
	req->host[i] = '\0';
	for (i = 0; *s != '\0'; i++, s++) {
		if (i > MAXURLSIZ) {
			fprintf(stderr, "Error: Cannot get remote file name from URL...\n");
			exit(1);
		}
		req->url[i] = *s;
	}
	req->url[i] = '\0';
	--s;
	for (i = 0; *s != '/'; i++, s--) {
		if (i > MAXBUFSIZ) {
			fprintf(stderr, "Error: Cannot get local file name from URL...\n");
			exit(1);
		}
		req->file[i] = *s;
	}
	req->file[i] = '\0';
	revstr(req->file);

	/* Required for FTP */
	if (strlen(req->username) <= 0)
		strncpy(req->username, ftpanonymoususer, MAXBUFSIZ - 2);
	if (strlen(req->password) <= 0)
		strncpy(req->password, ftpanonymouspass, MAXBUFSIZ - 2);
}

int numofthreads(int size)
{
	if (size == 0)
		return 0;
	else if (size < BUFSIZ * 4) 				/* < 16384 */	
		return 1;
	else if ((size >= BUFSIZ * 4) && (size < BUFSIZ * 8))	/* 16384 < x < 32678	*/ 
		return 2;
	else if ((size >= BUFSIZ * 8) && (size < BUFSIZ * 16))	/* 32768 < x < 65536	*/
		return 3;
	else if ((size >= BUFSIZ * 16) && (size < BUFSIZ * 32))	/* 65536 < x < 131072	*/
		return 4;
	else if ((size >= BUFSIZ * 32) && (size < BUFSIZ * 64))	/* 131072 < x < 262144	*/
		return 5;
	else if ((size >= BUFSIZ * 64) && (size < BUFSIZ * 128))	
		return 6;
	else if ((size >= BUFSIZ * 128) && (size < BUFSIZ * 256))	
		return 7;
	else if ((size >= BUFSIZ * 256) && (size < BUFSIZ * 512))	
		return 8;
	else if ((size >= BUFSIZ * 512) && (size < BUFSIZ * 1024))	
		return 9;
	else 
		return 10;
}

int calc_offset(int total, int part, int nthreads)
{
	return (part * (total / nthreads));
}

void usage()
{
	fprintf(stderr, "usage: aget [options] url\n");
	fprintf(stderr, "\toptions:\n");
	fprintf(stderr, "\t\t-p port number\n");
	fprintf(stderr, "\t\t-l local file name\n");
	fprintf(stderr, "\t\t-n suggested number of threads\n");
	fprintf(stderr, "\t\t-h this screen\n");
	fprintf(stderr, "\t\t-v version info\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "http//www.enderunix.org/aget/\n");
}

/* reverse a given string	*/
void revstr(char *str)
{
	char *p, *s;
	int i;
	int size;

	if ((size = strlen(str)) == 0)
		return;
	p = (char *)calloc(size+1, sizeof(char));
	s = p;
	for (i = size; i > 0; i--, s++)
		*s = *(str + i - 1);
	*s = '\0';
	memset(str, 0, size);
	strncpy(str, p, size);
	free(p);
}

/* Log	*/
void Log(char *fmt, ...)
{
	va_list ap;
	char *lfmt;

	lfmt = (char *)calloc(16 + strlen(fmt), sizeof(char));
	sprintf(lfmt, "Aget > %s", fmt);

	fflush(stdout);
	va_start(ap, fmt);
	vfprintf(stderr, lfmt, ap);
	va_end(ap);

	if (fmt[0] != '\0' && fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " %s", strerror(errno));
	fprintf(stderr, "\n");
	free(lfmt);
}

/* Progress Bar.
 * Modified to display progress every second using alarm().
 * If no progress has been made, nothing is printed for 5 successive calls
 * i.e., this guarantees display of progress bar every 5 seconds at least
 * and atmost every one second.
 */
void updateProgressBar()
{
	float rat;
	int ndot, i, j;
	static float prev = 0;
	char fmt[128];
	float cur = (float)bwritten;
	float tot = (float)req->clength;

	rat = cur/tot;
	ndot = (int)(rat * 100);

	if(ndot > 100)
		ndot = 100;
	for (i = 0; i < ndot; i += 2)
		putchar('=');
	putchar('>');
	i += 2;
	for (i = ndot - 1; i < 100; i += 2)
		putchar(' ');
	sprintf(fmt, "[%.2d%% done] Bytes: %u", ndot, bwritten);
	i += strlen(fmt);
	prev = ndot;
	printf("%s", fmt);
	/* Progress Bar should stay on only ONE line!	*/
	for (j = 0; j < i; j++)
		putchar(0x08);
	fflush(stdout);
}

void handleHttpRetcode(char *rbuf)
{
	if ((strstr(rbuf, "HTTP/1.1 416")) != NULL) {
		Log("Server returned HTTP/1.1 416 - Requested Range Not Satisfiable\n");
	        exit(1);
	} else
	if ((strstr(rbuf, "HTTP/1.1 403")) != NULL) {
		Log("<Server returned HTTP/1.1 403 - Permission Denied\n");
		exit(1);
	} else
	if ((strstr(rbuf, "HTTP/1.1 404")) != NULL) {
		Log("<Server returned HTTP/1.1 404 - File Not Found\n");
		exit(1);
	} 
}



/* Receive data from data connection (usually binary)
 * No point in displaying this!!!
 */
int recv_data(int sd, char *rbuf, int n)
{
	int nrecv;
	fd_set f;
	struct timeval tv;
	int ret, max=3;

 retry:
	if ((nrecv = recv(sd, rbuf, n, 0)) == -1 && errno!=EINTR) {
		Log("recv failed: %s", strerror(errno));
		exit(1);
	}
	if(nrecv == 0) { 	/* retry recv(), use a timeout	*/
		FD_ZERO(&f);
		FD_SET(sd, &f);
		tv.tv_sec = 1;	/* 1 second upperbound		*/
		tv.tv_usec = 0;
		if((ret = select(sd + 1, &f, NULL, NULL, &tv)) == -1) {
			Log("Error on select: %s", strerror(errno));
			exit(1);
		}
		if(ret > 0) {
			if(max--)
				goto retry;
		}
	}
	return nrecv;
}


/* Should be called only on error reply to display a suitable message	*/
void handleFTPRetcode(char *rbuf)
{
	if(rbuf[0] == '4') {
		if(rbuf[1] == '2')
			Log("There seems to be an error in the connections.");
		else if(rbuf[1] == '5')
			Log("Requested action not taken");
		Log("Retry the download after sometime.");
		Log("If the error persists, please report this with the command line used to murat@enderunix.org.");
	}
	else if(rbuf[0] == '5') {
		if(rbuf[1] == '0')
			Log("The server has reported a syntax error.");
		else if(rbuf[1] == '3')
			Log("Error relating to login; checkout the previous message, if available.");
		else if(rbuf[1] == '5')
			Log("Requested action not taken");
		Log("If you feel this is an error with aget, please report this with the command line used to murat@enderunix.org.");
	}
}	

/* This can handle multiline replies in FTP protocol by naveen n rao	*/
int recv_reply(int sd, char *rbuf, int len, int flags)
{
	char line[1024];
	FILE *fp;
	int nread;
	int done;
	char status[4] = {0};

	if ((fp = fdopen(sd, "r")) == NULL)
		return -1;

	nread = 0;
	done = 0;
	do {
		fgets(line, 1020, fp);
		if (line[3] == '-' && status[0] == 0) 
			strncpy(status, line, 3);

		if (line[3] == ' ') {
			if (status[0] != 0) {
				if (strncmp(status, line, 3) == 0)
					done = 1;
			}
			else
				done = 1;
		}
		memcpy(rbuf + nread, line, strlen(line));
		nread += strlen(line);
	} while (done != 1 && nread < 1022);

#ifdef DEBUG
	printf("READ: %d\n", nread);
	fwrite(rbuf, 1, nread, stderr);
#endif
	return nread;
}
