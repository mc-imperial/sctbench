
/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 * $Id: Resume.c,v 1.7 2003/01/16 20:15:23 rao Exp $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>

#include "Data.h"
#include "Resume.h"
#include "Misc.h"

 #include <assert.h>

extern struct thread_data *wthread;
extern struct request *req;
extern int nthreads;
extern int bwritten;
extern time_t t_start, t_finish;

void save_log()
{
	char *logfile, *s;
	struct hist_data h;
	FILE *fp;
	time_t diff_sec;
	struct passwd *pw;

	long total_bwritten;
	int i;

	if(wthread == NULL)	// Do nothing
		return;

	if(bwritten == req->clength) {	// If download has just completed
		time(&t_finish);
       		if ((diff_sec = t_finish - t_start) == 0)
			diff_sec = 1;   /* Avoid division by zero */

		Log("Download already complete: job completed in %d seconds. (%d Kb/sec)",
				diff_sec, (req->clength / diff_sec) / 1024);
        	Log("Shutting down...");
		return;
	}

	logfile = (char *)calloc(255, sizeof(char));

	/* Try and save log file in user's home directory */
	//pw = getpwuid(getuid());
	//if(pw->pw_dir == NULL)
		/* A small fix: save protocol in log filename.
		 * This is necessary in case urls for ftp and http locations of
		 * a file is the same -- rare, but happens to me all the time (localhost)
		 */
		snprintf(logfile, 255, "%s-aget%c.log", req[0].lfile, req->proto == PROTO_HTTP ? 'h' : 'f');
	//else {
	//	s = &req->lfile[strlen(req->lfile)-1];
	//	while(*s!='/' && s!=req->lfile)
	//		s--;
	//	snprintf(logfile, 255, "%s%s-aget%c.log", pw->pw_dir, s, req->proto == PROTO_HTTP ? 'h' : 'f');
	//}

	//if ((fp = fopen(logfile, "w")) == NULL) {
	//	fprintf(stderr, "Cannot open log file %s for writing: %s\n", logfile, strerror(errno));
	//	exit(1);
	//}
	
	/* Get the finish time, derive some stats */
	time(&t_finish);
       	if ((diff_sec = t_finish - t_start) == 0)
		diff_sec = 1;   /* Avoid division by zero */
	req->time_taken += diff_sec;

	memcpy(&(h.req), req, sizeof(struct request));
	memcpy(&(h.wthread), wthread, sizeof(struct thread_data) * nthreads);
	h.nthreads = nthreads;
	h.bwritten = bwritten;

	printf("--> Logfile is: %s, so far %d bytes have been transferred\n", logfile, h.bwritten);

	//fwrite(&h, sizeof(struct hist_data), 1, fp);
	//fclose(fp);

	free(logfile);

	total_bwritten = 0;
    for (i = 0; i < h.nthreads; i++) {
		total_bwritten += (h.wthread[i].offset - h.wthread[i].soffset);
	}
	printf("total bytes: %d, log file total bytes: %d\n", total_bwritten, h.bwritten);
	assert(total_bwritten == h.bwritten);
}

int read_log(struct hist_data *h)
{
	char *logfile, *s;
	FILE *fp;
	struct passwd *pw;

	logfile = (char *)calloc(255, sizeof(char));

	/* First see if the log file is in present directory, if not check user's home directory */
	snprintf(logfile, 255, "%s-aget%c.log", req[0].lfile, req->proto == PROTO_HTTP ? 'h' : 'f');

	Log("Attempting to read log file for resuming download job...");

	if ((fp = fopen(logfile, "r")) == NULL) {
		if (errno == ENOENT) {
			pw = getpwuid(getuid());
			if(pw->pw_dir != NULL) {
				s = &req->lfile[strlen(req->lfile)-1];
				while(*s!='/' && s!=req->lfile)
					s--;
				snprintf(logfile, 255, "%s%s-aget%c.log", pw->pw_dir, s, req->proto == PROTO_HTTP ? 'h' : 'f');
				if ((fp = fopen(logfile, "r")) == NULL) {
					if (errno == ENOENT) {
						Log("Couldn't find log file for this download, starting a clean job...");
						return -1;
					} else {
						fprintf(stderr, "Cannot open log file %s for reading: %s\n", logfile, strerror(errno));
						exit(1);
					}
				}
			}
			else {
				Log("Couldn't find log file for this download, starting a clean job...");
				return -1;
			}
		} else {
			fprintf(stderr, "Cannot open log file %s for reading: %s\n", logfile, strerror(errno));
			exit(1);
		}
	}

	fread(h, sizeof(struct hist_data), 1, fp);
	bwritten = h->bwritten;
	fclose(fp);

	Log("%d bytes already transferred", bwritten);

	/* Unlinking logfile after we've read it. */
	if ((unlink(logfile)) == -1)
		fprintf(stderr, "read_log: cannot remove stale log file %s: %s\n", logfile, strerror(errno));

	free(logfile);

	return 0;
}
