
/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Head.h"
#include "Aget.h"
#include "Misc.h"
#include "Download.h"
#include "Ftp.h"
#include "Resume.h"
#include "Data.h"
#include "Signal.h"

/*#define DEBUG	*/

extern struct thread_data *wthread;
extern struct request *req;
extern int fsuggested, nthreads;
extern int bwritten;
extern pthread_t hthread;
extern char http_proxyhost[VALSIZE];

time_t  t_start, t_finish;

void startHTTP(struct request *req)
{
	int i, ret, fd, diff_sec, nok = 0;
	long soffset, foffset;
	char *fmt;
	sigset_t set;

	sigfillset(&set);
	//http_head_req(req);
	strcpy(req->host, "local");
	req->port = 80;
	strcpy(req->url, "gnu.txt");
	req->clength = 35147;

	/* TODO:: We should create the helper thread here, but should take care
	 * while resuming since wthread structure may not be allocated at all
	 */

	/* According to the content-length, get the
	 * suggested number of threads to open.
	 * if the user insists on his value, let it be that way,
	 * use the user's value.
	 */
	ret = numofthreads(req->clength);
	if (fsuggested == 0) {
		if (ret == 0)
			nthreads = 1;
		else
			nthreads = ret;
	}
	wthread = (struct thread_data *)calloc(nthreads, sizeof(struct thread_data));
	Log("Downloading %s (%d bytes) from site %s(%s:%d).\nNumber of Threads: %d",
			req->url, req->clength, req->host, req->ip, req->port, nthreads);

	if ((fd = open(req->lfile, O_CREAT | O_RDWR | O_EXCL, S_IRWXU)) == -1) {
		if(errno == EEXIST) {
			char reply[MAXBUFSIZ];
again:
			fprintf(stderr, "File already exists! Overwrite?(y/n) ");
			//scanf("%2s", reply);
			
			//if(reply[0] == 'n')
			//	exit(1);
			//else if(reply[0] == 'y') {
				if ((fd = open(req->lfile, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
					fprintf(stderr, "get: cannot open file %s for writing: %s\n", req->lfile, strerror(errno));
					exit(1);
				}
			//}
			//else
			//	goto again;
		}
	}

	if ((lseek(fd, req->clength - 1, SEEK_SET)) == -1) {
		fprintf(stderr, "get: couldn't lseek:  %s\n", strerror(errno));
		exit(1);
	}

	if ((write(fd, "0", 1)) == -1) {
		fprintf(stderr, "get: couldn't allocate space for download file: %s\n", strerror(errno));
		exit(1);
	}

	/* Get the starting time, prepare GET format string, and start the threads */
	fmt = (char *)calloc(GETREQSIZ - 2, sizeof(char));
	time(&t_start);
retry:
	for (i = 0; i < nthreads && wthread[i].status | STAT_OK; i++) {
		soffset = calc_offset(req->clength, i, nthreads);
		foffset = calc_offset(req->clength, i + 1, nthreads);
		wthread[i].soffset = soffset;
		wthread[i].foffset = (i == nthreads - 1 ? req->clength : foffset);
		wthread[i].offset = wthread[i].soffset;
		wthread[i].sin.sin_family = AF_INET;
		wthread[i].sin.sin_addr.s_addr = inet_addr(req->ip);
		wthread[i].sin.sin_port = htons(req->port);
		wthread[i].fd = dup(fd);
		wthread[i].clength = req->clength;
		if(strlen(http_proxyhost) > 0) {
			char tmp[MAXBUFSIZ];
			if(req->proto == PROTO_HTTP) {
				strcpy(tmp, "http://");
			} else {
				strcpy(tmp, "ftp://");
			}
			strncat(tmp, req->host, MAXBUFSIZ);
			strncat(tmp, req->url, MAXBUFSIZ);
			snprintf(fmt, GETREQSIZ, GETREQ, tmp, req->host, PROGVERSION, soffset);
		} else {
			snprintf(fmt, GETREQSIZ, GETREQ, req->url, req->host, PROGVERSION, soffset);
		}
		strncpy(wthread[i].getstr, fmt, GETREQSIZ);
#ifdef DEBUG
		printf("Start: %ld, Finish: %ld, Offset: %ld, Diff: %ld\n",
				wthread[i].soffset,
				wthread[i].foffset,
				wthread[i].offset,
				wthread[i].offset - wthread[i].soffset);
#endif
	}

	/* Block out all signals */
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	/* Create a thread for handling signals.
	 * Should create the helper thread only after wthread structure is ready & sane.
	 * Only then does saving the log file makes sense.
	 */
	if ((ret = pthread_create(&hthread, NULL, signal_waiter, NULL)) != 0) {
		fprintf(stderr, "startHTTP: cannot create signal_waiter thread: %s, exiting...\n", strerror(errno));
		exit(-1);
	}

	for(i = 0; i < nthreads && wthread[i].status | STAT_OK; i++) {
		if(pthread_create(&(wthread[i].tid), NULL, http_get, &(wthread[i])) != 0) {
			Log("Thread creation error!");
			exit(2);
		}
		pthread_testcancel();	/* Check if SIGINT caught & exit -- safe cancellation point */
	}

	pthread_testcancel();

	/* Wait for all of the threads to finish... */
	for (i = 0; i < nthreads; i++) {
		if((int)wthread[i].tid != -1) {
			pthread_join(wthread[i].tid, NULL);
			if (wthread[i].status == STAT_OK)
				nok++;
		}

		/* Now set tid=-1, so that if an interrupt arrives midway through this loop,
		 * signal handler won't wait for the same thread
		 */
		wthread[i].tid = -1;
		pthread_testcancel();
	}

	//if (nok == nthreads) {
	//	/* Block out all signals */
	//	pthread_sigmask(SIG_BLOCK, &set, NULL);
	//	pthread_cancel(hthread);
	//}
	//else	/* Restart threads that failed */
	//	goto retry;

	free(fmt);

	/* Get the finish time, derive some stats */
	time(&t_finish);
       	if ((diff_sec = t_finish - t_start) == 0)
		diff_sec = 1;   /* Avoid division by zero */

	Log("Download completed, job completed in %d seconds. (%d Kb/sec)",
			diff_sec, (req->clength / diff_sec) / 1024);

        Log("Shutting down...");
	close(fd);
}

void startFTP(struct request *req)
{
	int i, ret, fd, diff_sec, nok = 0;
	long soffset, foffset;
	int head_sd;
	sigset_t set;

	sigfillset(&set);

	if(strlen(http_proxyhost) > 0) {
		startHTTP(req);
	}
	
	/* Should return the socket descriptor for the control connection in sd_c
 	 * and for the data connection in sd_d so that we can reuse it. That
	 * reduces initial logon overhead. Currently returns a flag indicating
	 * whether the server supports RESTart command.
	 */
	head_sd = -1;
	if((ret = ftp_head_req(req, &head_sd)) == 1) {
		/* REST command supported
		 * According to the content-length, get the
		 * suggested number of threads to open.
		 * if the user insists on his value, let it be that way,
		 * use the user's value.
		 */
		ret = numofthreads(req->clength);
		if (fsuggested == 0) {
			if (ret == 0)
				nthreads = 1;
			else
				nthreads = ret;
		}
	} else
		nthreads = 1;

	/* TODO:: We should create the helper thread here, but should take care
	 * while resuming since wthread structure may not be allocated at all
	 */

	wthread = (struct thread_data *)calloc(nthreads, sizeof(struct thread_data));
	Log("Downloading %s (%d bytes) from site %s(%s:%d).\nNumber of Threads: %d",
			req->url, req->clength, req->host, req->ip, req->port, nthreads);

	if ((fd = open(req->lfile, O_CREAT | O_RDWR | O_EXCL, S_IRWXU)) == -1) {
		if(errno == EEXIST) {
			char reply[MAXBUFSIZ];
again:
			fprintf(stderr, "File already exists! Overwrite?(y/n) ");
			scanf("%2s", reply);
			
			if(reply[0] == 'n')
				exit(1);
			else if(reply[0] == 'y') {
				if ((fd = open(req->lfile, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
					fprintf(stderr, "get: cannot open file %s for writing: %s\n", req->lfile, strerror(errno));
					exit(1);
				}
			}
			else
				goto again;
		}
	}

	if ((lseek(fd, req->clength - 1, SEEK_SET)) == -1) {
		fprintf(stderr, "get: couldn't lseek:  %s\n", strerror(errno));
		exit(1);
	}

	if ((write(fd, "0", 1)) == -1) {
		fprintf(stderr, "get: couldn't allocate space for download file: %s\n", strerror(errno));
		exit(1);
	}

	/* Get the starting time and start the threads */
	time(&t_start);
retry:
	for (i = 0; i < nthreads && wthread[i].status | STAT_OK; i++) {
		soffset = calc_offset(req->clength, i, nthreads);
		foffset = calc_offset(req->clength, i + 1, nthreads);
		wthread[i].head_sd = (i == 0 ? head_sd : -1);
		wthread[i].soffset = soffset;
		wthread[i].foffset = (i == nthreads - 1 ? req->clength : foffset);
		wthread[i].offset = wthread[i].soffset;
		wthread[i].sin.sin_family = AF_INET;
		wthread[i].sin.sin_addr.s_addr = inet_addr(req->ip);
		wthread[i].sin.sin_port = htons(req->port);
		wthread[i].fd = dup(fd);
		wthread[i].clength = req->clength;
		strncpy(wthread[i].username, req->username, MAXBUFSIZ-1);
		strncpy(wthread[i].password, req->password, MAXBUFSIZ-1);
		strncpy(wthread[i].url, req->url, MAXURLSIZ -1);
#ifdef DEBUG
		printf("Start: %ld, Finish: %ld, Offset: %ld, Diff: %ld\n",
				wthread[i].soffset,
				wthread[i].foffset,
				wthread[i].offset,
				wthread[i].offset - wthread[i].soffset);
#endif
	}
	
	/* Block out all signals */
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	/* Create a thread for handling signals.
	 * Should create the helper thread only after wthread structure is ready & sane.
	 * Only then does saving the log file makes sense.
	 */
	if ((ret = pthread_create(&hthread, NULL, signal_waiter, NULL)) != 0) {
		fprintf(stderr, "main: cannot create signal_waiter thread: %s, exiting...\n", strerror(errno));
		exit(-1);
	}

	for(i = 0; i < nthreads && wthread[i].status | STAT_OK; i++) {
		if(pthread_create(&(wthread[i].tid), NULL, ftp_get, &(wthread[i])) != 0) {
			Log("Thread creation error!");
			exit(2);
		}
		pthread_testcancel();	/* Check if SIGINT caught & exit -- safe cancellation point */
	}

	pthread_testcancel();

	/* Wait for all of the threads to finish... */
	for (i = 0; i < nthreads; i++) {
		if((int)wthread[i].tid != -1) {
			pthread_join(wthread[i].tid, NULL);
			if (wthread[i].status == STAT_OK)
				nok++;
		}
		/* Now set tid=-1, so that if an interrupt arrives midway through this loop,
		 * signal handler won't wait for the same thread
		 */
		wthread[i].tid = -1;
		pthread_testcancel();
	}

	if (nok == nthreads) {
		/* Block out all signals */
		pthread_sigmask(SIG_BLOCK, &set, NULL);
		pthread_cancel(hthread);
	}
	else	/* Restart threads that failed */
		goto retry;

	/* Get the finish time, derive some stats */
	time(&t_finish);
       	if ((diff_sec = t_finish - t_start) == 0)
		diff_sec = 1;   /* Avoid division by zero */

	Log("Download completed, job completed in %d seconds. (%d Kb/sec)",
			diff_sec, (req->clength / diff_sec) / 1024);
        Log("Shutting down...");
	close(fd);
}

/*
 Can be used for resuming any download
*/
void resumeDownload(struct hist_data *h, int proto)
{
	int i, fd, diff_sec, nok = 0, ret;
	char *fmt;
	sigset_t set;

	sigfillset(&set);
	nthreads = h->nthreads;
	fmt = (char *)calloc(GETREQSIZ - 2, sizeof(char));
	wthread = (struct thread_data *)malloc(nthreads * sizeof(struct thread_data));
	memcpy(req, &h->req, sizeof(struct request));
	memcpy(wthread, h->wthread, sizeof(struct thread_data) * nthreads);

	Log("Resuming download %s (%d bytes) from site %s(%s:%d).\nNumber of Threads: %d",
			req->url, req->clength, req->host, req->ip, req->port, nthreads);

	if ((fd = open(req->lfile, O_RDWR, S_IRWXU)) == -1) {
		fprintf(stderr, "get: cannot open file %s for writing: %s\n", req->lfile, strerror(errno));
		exit(1);
	}

	time(&t_start);

retry:
	for (i = 0; i < nthreads && wthread[i].status | STAT_OK; i++) {
		wthread[i].head_sd = -1;
		wthread[i].soffset = wthread[i].offset;
		wthread[i].fd = dup(fd);
		snprintf(fmt, GETREQSIZ, GETREQ, req->url, req->host, PROGVERSION, wthread[i].offset);
		strncpy(wthread[i].getstr, fmt, GETREQSIZ);
#ifdef DEBUG
		printf("Start: %ld, Finish: %ld, Offset: %ld, Diff: %ld\n",
				wthread[i].soffset,
				wthread[i].foffset,
				wthread[i].offset,
				wthread[i].offset - wthread[i].soffset);
#endif
	}

	/* Block out all signals */
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	/* Create a thread for handling signals.
	 * Should create the helper thread only after wthread structure is ready & sane.
	 * Only then does saving the log file makes sense.
	 */
	if ((ret = pthread_create(&hthread, NULL, signal_waiter, NULL)) != 0) {
		fprintf(stderr, "main: cannot create signal_waiter thread: %s, exiting...\n", strerror(errno));
		exit(-1);
	}

	for(i = 0; i < nthreads && wthread[i].status | STAT_OK; i++) {
		if(proto == PROTO_FTP)
			pthread_create(&(wthread[i].tid), NULL, ftp_get, &(wthread[i]));
		else if(proto == PROTO_HTTP)
			pthread_create(&(wthread[i].tid), NULL, http_get, &(wthread[i]));
		pthread_testcancel();
	}

	pthread_testcancel();

	for (i = 0; i < nthreads; i++) {
		if((int)wthread[i].tid != -1) {
			pthread_join(wthread[i].tid, NULL);
			if (wthread[i].status == STAT_OK)
				nok++;
		}
		/* Now set tid=-1, so that if an interrupt arrives midway through this loop,
		 * signal handler won't wait for the same thread
		 */
		wthread[i].tid = -1;
		pthread_testcancel();
	}

	if (nok == nthreads) {
		/* Block out all signals */
		pthread_sigmask(SIG_BLOCK, &set, NULL);
		pthread_cancel(hthread);
	}
	else	/* Restart threads that failed */
		goto retry;

        time(&t_finish);
        if ((diff_sec = t_finish - t_start) == 0)
		diff_sec = 1;   /* Avoid division by zero */
	Log("Download completed, job completed in %d seconds. (%d Kb/sec)",
			diff_sec, ((req->clength - h->bwritten) / diff_sec) / 1024);
	Log("Total download time: %d seconds. Overall speed: %d Kb/sec",
			diff_sec+req->time_taken, (req->clength/(diff_sec+req->time_taken)) / 1024);
        Log("Shutting down...");
	close(fd);
}
