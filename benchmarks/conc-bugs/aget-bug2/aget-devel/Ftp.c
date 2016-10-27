
/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 *
 * Nearly all Ftp code in Ftp.c, Misc.c and Head.c has been contributed by
 * Naveen N Rao.
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
#include <errno.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>

#include "Head.h"
#include "Data.h"
#include "Defs.h"
#include "Misc.h"
#include "Ftp.h"

extern sigset_t signal_set;
extern unsigned int bwritten;
extern pthread_mutex_t bwritten_mutex;

int parse_pasv_reply2(char *, int, struct sockaddr_in *);
int parse_list_reply(char *rbuf, int len);
int ftp_get_size(struct sockaddr_in *sin, char *url, int type);

void *ftp_get(void *arg) {
	struct sockaddr_in sin_dc;
	struct thread_data *td;
	int sd_c, sd_d;
	char *sbuf, *rbuf;
	int dr, dw;
	long foffset;
	sigset_t set;
	pthread_t tid;

	tid = pthread_self();
	/* Block out all signals */
	sigfillset(&set);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	/* Set Cancellation Type to Asynchronous */
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	td = (struct thread_data *)arg;
	foffset = td->foffset;
	
	if(td->soffset < 0 || td->soffset >= td->foffset) {	/* If download complete */
		td->status = STAT_OK;		/* Tell that download is OK. */
		pthread_exit((void *)1);
		return NULL;
	}

	rbuf = (char *)calloc(MAXBUFSIZ, sizeof(char));
	sbuf = (char *)calloc(MAXBUFSIZ, sizeof(char));

	if (td->head_sd == -1) {
		if ((sd_c = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			Log("<THREAD #%ld> socket creation failed: %s", tid, strerror(errno));
			pthread_exit((void *)1);
		}
	
		if ((connect(sd_c, (const struct sockaddr *)&td->sin, sizeof(struct sockaddr))) == -1) {
			Log("<THREAD #%ld> connection failed: %s", tid, strerror(errno));
			pthread_exit((void *)1);
		}
	
		/* Should receive manually, cause some servers may not accept any connections!	*/
		if(recv_reply(sd_c, rbuf, MAXBUFSIZ, 0) == -1) {
			fprintf(stderr, "<<<<<----- %s", rbuf);
			Log("recv failed: %s", strerror(errno));
			pthread_exit((void *)1);
		}
		else if(rbuf[0] != '2' && rbuf[0] != '1' && rbuf[0] != '3') {
			fprintf(stderr, "<<<<<----- %s", rbuf);
			Log("Seems like the server isn't accepting connections now, bailing out...");
			pthread_exit((void *)1);
		}

		if(rbuf[0] != '1' && rbuf[0] != '2' && rbuf[0] != '3') {
			fprintf(stderr, rbuf);
			handleFTPRetcode(rbuf);
			close(sd_c);
			pthread_exit((void *)1);
		}
	
		snprintf(sbuf, MAXBUFSIZ - 1, "USER %s\r\n", td->username);
		memset(rbuf, 0, MAXBUFSIZ);
		send(sd_c, sbuf, strlen(sbuf), 0);
		recv_reply(sd_c, rbuf, MAXBUFSIZ, 0);
		if (strncmp(rbuf, "331", 3) != 0) {
			fprintf(stderr, "Server didnot like username, server said\n");
			fprintf(stderr, "Server > %s\n", rbuf);
			exit(1);
		}

		snprintf(sbuf, MAXBUFSIZ - 1,"PASS %s\r\n", td->password);
		memset(rbuf, 0, MAXBUFSIZ);
		send(sd_c, sbuf, strlen(sbuf), 0);
		recv_reply(sd_c, rbuf, MAXBUFSIZ, 0);
		if (strncmp(rbuf, "230", 3) != 0) {
			fprintf(stderr, "Server didnot accept password, server said\n");
			fprintf(stderr, "Server > %s\n", rbuf);
			exit(1);
		}
	} else {
		sd_c = td->head_sd;
		recv_reply(sd_c, rbuf, MAXBUFSIZ, 0);
	}

	snprintf(sbuf, MAXBUFSIZ - 1, "REST %ld\r\n", td->soffset);
	memset(rbuf, 0, MAXBUFSIZ);
	send(sd_c, sbuf, strlen(sbuf), 0);
	recv_reply(sd_c, rbuf, MAXBUFSIZ, 0);
	if (strncmp(rbuf, "350", 3) != 0) {
		fprintf(stderr, "Server doesn't accept resuming transfers.");
		fprintf(stderr, "Server > %s\n", rbuf);
		exit(1);
	}

	sprintf(sbuf, "TYPE I\r\n");
	memset(rbuf, 0, MAXBUFSIZ);
	send(sd_c, sbuf, strlen(sbuf), 0);
	recv_reply(sd_c, rbuf, MAXBUFSIZ, 0);
	if (strncmp(rbuf, "200", 3) != 0) {
		fprintf(stderr, "Server didnot accept BINARY transfer.\n");
		fprintf(stderr, "Server > %s\n", rbuf);
	}

	sprintf(sbuf, "PASV\r\n");
	memset(rbuf, 0, MAXBUFSIZ);
	send(sd_c, sbuf, strlen(sbuf), 0);
	recv_reply(sd_c, rbuf, MAXBUFSIZ, 0);
	parse_pasv_reply2(rbuf, MAXBUFSIZ, &sin_dc);


	if ((sd_d = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		Log("Socket creation failed: %s", strerror(errno));
		exit(1);
	}
	if ((connect(sd_d, (const struct sockaddr *)&sin_dc, sizeof(sin_dc))) == -1) {
		Log("Connection failed: %s", strerror(errno));
		exit(1);
	}

	/* Should avoid preceding '/'	*/
	snprintf(sbuf,  MAXBUFSIZ - 1,"RETR %s\r\n", td->url + 1);
	memset(rbuf, 0, MAXBUFSIZ);
	send(sd_c, sbuf, strlen(sbuf), 0);

	td->offset = td->soffset;
	
	while (td->offset < foffset) {
		/* Set Cancellation Type to Asynchronous
		 * in case user needs to cancel recv().
		 * recv() is a blocking call.
		 */
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

		memset(rbuf, MAXBUFSIZ, 0);
		dr = recv_data(sd_d, rbuf, MAXBUFSIZ);

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
		td->status = STAT_OK;		/* Tell that download is OK. */
	close(sd_c);
	close(sd_d);

	pthread_exit(NULL);
	return NULL;
}
