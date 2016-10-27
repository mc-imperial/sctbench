
/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 * $Id: main.c,v 1.14 2004/06/01 06:42:21 murat Exp $
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>

#include "Defs.h"
#include "Data.h"
#include "Misc.h"
#include "Aget.h"
#include "Signal.h"
#include "Resume.h"

#define VALSIZE 256


extern char ftpanonymoususer[VALSIZE];
extern char ftpanonymouspass[VALSIZE];
extern int preferredthread;
extern char http_proxyhost[VALSIZE];
extern char http_proxyuser[VALSIZE];
extern char http_proxypass[VALSIZE];
extern int http_proxyport;

extern void readrc();


int nthreads;
int fsuggested;
struct request *req;		/* Download jobs		*/
pthread_t hthread;		/* Helper thread for signals	*/
struct thread_data *wthread;	/* Worker Threads		*/
pthread_t main_tid;

int main(int argc, char **argv)
{
	int c, error = 0;
	struct hist_data h;
	int retlog;
	char tmp[MAXBUFSIZ];
	char *fullurl;
	extern char *optarg;
	extern int optind;
	sigset_t set;

	main_tid = pthread_self();
	/* Allocate heap for download request	
	 * struct request stores all the information that might be
	 * of interest
	 */
	req = (struct request *)calloc(1, sizeof(struct request));

	/* This is required because if download is aborted before
	 * this is initialized, problems will occur
	 */
	wthread = NULL;

	/* except from signal handler thread, 
	 * no other thread is receiving signals!
	 * No signals are caught here since we need to save download job only
	 * after wthread structures are initialized.
	 * TODO:: We need to create helper thread as soon as head request is over
	 * since the user might wish to stop downloading as soon as he sees the
	 * content length.
	 */
	sigfillset(&set);
	pthread_sigmask(SIG_BLOCK, &set, NULL);

	/* Safe to exit before download starts.
	 * This is set back to DEFERRED before creating helper thread because
	 * we want this to exit only at certain safe points specified using
	 * pthread_testcancel().
	 * Helper thread is started just before creating worker threads and after
	 * wthread structures are initialized. Only after this point do we need to
	 * save download job if an interrupt comes - not before. Helper thread exists
	 * only to catch SIGINT and to save download job. It doesn't make sense to
	 * start it before wthread structures are initialized!
	 */
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);


	/* Read the RC file if exist	*/
	readrc();


	/* If set in rc file	*/
	if (preferredthread != -1)
		nthreads = preferredthread;

	while (!error && (c = getopt(argc,argv,"p:l:n:hfv")) != -1) {
		switch(c) {
			case 'p':
				req->port = atoi(optarg);
				break;
			case 'l':
				strncpy(req->lfile, optarg, MAXBUFSIZ -1);
				break;
			case 'n':
				fsuggested = 1;
				if ((nthreads = atoi(optarg)) > MAXTHREADS) {
					Log("Error: Maximum # of threads allowed is %d\n", MAXTHREADS);
					exit(1);
				}
				break;
			case 'h':
				printf("%s\n", PROGVERSION);
				usage();
				exit(0);
				break;
			case 'v':
				printf("%s\nby Murat BALABAN <murat@enderunix.org>\n", PROGVERSION);
				exit(0);
				break;
			default:
				error = 1;
				usage();
				exit(1);
				break;
		}
	}

	if (error) {
		usage();
		exit(1);
	}

	if (fsuggested == 1 && nthreads == 0) {
		fprintf(stderr, "ERROR: -f and -n should be used together!, exiting...\n");
		usage();
		exit(1);
	}

	if (argc == 2) 		/* If only url is supplied... */
		fullurl = strdup(argv[1]);
	else
	if (optind < argc)
		if (argc > 2)
			fullurl = strdup(argv[optind]);
		else {
			usage();
			exit(1);
		}
	else
	if (optind == argc) {
		usage();
		exit(1);
	}

	parse_url(fullurl, req);


	if(strlen(req->lfile) == 0)
		strncpy(req->lfile, req->file, MAXBUFSIZ - 2);

	getcwd(tmp, MAXBUFSIZ -2);
	strncpy(tmp+strlen(tmp), "/", MAXBUFSIZ - strlen(tmp) - 2);
	strncpy(tmp+strlen(tmp), req->lfile, MAXBUFSIZ - strlen(tmp) - 3);
	strncpy(req->lfile, tmp, MAXBUFSIZ - 2);

	/* If a log file for a previous try has been found, read it and
	 * resume the download job (resume_get), otherwise, start with
	 * a clean job (get)
	 *
	 * Logfile is of the pattern: aget-$file_name.log
	 */
	switch(req->proto) {
		case PROTO_HTTP:
			if ((retlog = read_log(&h)) != -1)
				resumeDownload(&h, PROTO_HTTP);
			else
				startHTTP(req);
			break;
		case PROTO_FTP:
			if ((retlog = read_log(&h)) != -1)
				resumeDownload(&h, PROTO_FTP);
			else
				startFTP(req);
			break;
	}
	return 0;
}
