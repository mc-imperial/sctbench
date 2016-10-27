
/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 *
 */

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "Head.h"
#include "Data.h"
#include "Defs.h"
#include "Misc.h"
#include "Download.h"

extern sigset_t signal_set;


#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

int socket_m(int domain, int type, int protocol, long soffset)
{
	char filename[200];
	sprintf(filename, "%ld", soffset);
	int fd = open(filename, O_RDONLY);
	assert(fd != -1);
	return fd;
}

int connect_m(int socket, const struct sockaddr *address,
       socklen_t address_len)
{
	return 0;
}

ssize_t send_m(int sockfd, const void *buf, size_t len, int flags)
{
	return len;
}


// ssize_t recv_capture(int socket, void *buffer, size_t length, int flags, long soffset)
// {
// 	ssize_t res = recv(socket, buffer, length, flags);

// 	FILE *file;
// 	char filename[200];
// 	sprintf(filename, "%ld", soffset);
// 	file = fopen(filename,"a+");
// 	size_t pos;
// 	for(pos = 0; pos < res; ++pos) {
// 		fputc(((char*)buffer)[pos], file);
// 	}
// 	fclose(file);


// 	return res;
// }

ssize_t recv_m(int socket, void *buffer, size_t length, int flags)
{
	ssize_t res = read(socket, buffer, length);
	return res;
}


unsigned int bwritten = 0;
pthread_mutex_t bwritten_mutex = PTHREAD_MUTEX_INITIALIZER;

void *http_get(void *arg) {
	struct thread_data *td;
	int sd;
	char *rbuf, *s;
	int dr, dw, i; 
	long foffset;
	pthread_t tid;
	sigset_t set;

	tid = pthread_self();
	/* Block out all signals */
	sigfillset(&set);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	/* Set Cancellation Type to Asynchronous */
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	td = (struct thread_data *)arg;
	foffset = td->foffset;
	
	if(td->soffset < 0 || td->soffset >= td->foffset) { /* If download complete */
		td->status = STAT_OK;		/* Tell that download is OK. */
		pthread_exit((void *)1);
		return NULL;
	}

	rbuf = (char *)calloc(MAXBUFSIZ, sizeof(char));
	if ((sd = socket_m(AF_INET, SOCK_STREAM, 0, td->soffset)) == -1) {
		Log("<THREAD #%ld> socket creation failed: %s", tid, strerror(errno));
		pthread_exit((void *)1);
	}

	if ((connect_m(sd, (const struct sockaddr *)&td->sin, sizeof(struct sockaddr))) == -1) {
		Log("<THREAD #%ld> connection failed: %s", tid, strerror(errno));
		pthread_exit((void *)1);
	}

	if ((send_m(sd, td->getstr, strlen(td->getstr), 0)) == -1) {
		Log("<THREAD #%ld> send failed: %s", tid, strerror(errno));
		pthread_exit((void *)1);
	}

        if ((dr = recv_m(sd, rbuf, MAXBUFSIZ-1, 0)) == -1) {
		Log("<THREAD #%ld> recv failed: %s", tid, strerror(errno));
		pthread_exit((void *)1);
        }
	rbuf[dr] = '\0';

	handleHttpRetcode(rbuf);
        if ((strstr(rbuf, "HTTP/1.1 206")) == NULL) {
		fprintf(stderr, "Something unhandled happened, shutting down...\n");
		exit(1);
	}
	s = rbuf;
	i = 0;
	while(1 && (s - rbuf) < dr) {
		if (*s == '\n' && *(s - 1) == '\r' && *(s - 2) == '\n' && *(s - 3) == '\r' && (s - rbuf) < dr) {
			s++;
			i++;
			break;
		}
		s++;
		i++;
	}

	if ((dr - i ) > foffset)
		dw = pwrite(td->fd, s, (foffset - i), td->soffset);
	else
		dw = pwrite(td->fd, s, (dr - i), td->soffset);
	td->offset = td->soffset + dw;
	pthread_mutex_lock(&bwritten_mutex);
	bwritten += dw;
	pthread_mutex_unlock(&bwritten_mutex);

	pthread_testcancel();	/* Check for pending cancel requests */

	while (td->offset < foffset) {
		fd_set set;
		/* Set Cancellation Type to Asynchronous
		 * so that a blocking recv() doesn't cause problems
		 */
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

		/* patch by Gurer Ozen	
		 * in slow downloads, avoids excessive CPU consuming...
		 */
		//FD_ZERO(&set);
		//FD_SET(sd,&set);
		//select(FD_SETSIZE,&set,NULL,NULL,NULL);
		/* --end of patch	*/

		memset(rbuf, 0, MAXBUFSIZ);
		dr = recv_m(sd, rbuf, MAXBUFSIZ, 0);

		/* Set Cancellation Type back to Deferred */
		pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

		if ((td->offset + dr) > foffset)
			dw = pwrite(td->fd, rbuf, foffset - td->offset, td->offset);
		else
			dw = pwrite(td->fd, rbuf, dr, td->offset);
		td->offset += dw;

		pthread_mutex_lock(&bwritten_mutex);
		bwritten += dw;
		pthread_mutex_unlock(&bwritten_mutex);
		pthread_testcancel();	/* Check for pending cancel requests */
	}

	if (td->offset == td->foffset)
		td->status = STAT_OK;		/* Tell thet download is OK. */
	close(sd);
	pthread_exit(NULL);
	return NULL;
}
