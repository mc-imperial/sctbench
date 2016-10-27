/*
 * Aget, Multithreaded Download Accelerator
 *
 * (c) 2002 Murat Balaban <murat at enderunix dot org>
 * See COPYING for copyright information
 *
 *
 */

#define _XOPEN_SOURCE 500
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#ifdef SOLARIS
#include <arpa/nameser.h>
#include <resolv.h>
#endif


#include "Head.h"
#include "Data.h"
#include "Defs.h"
#include "Misc.h"


extern int h_errno;
extern char http_proxyhost[VALSIZE];
extern int http_proxyport;


int parse_pasv_reply(char *, int, struct sockaddr_in *);
int parse_pasv_reply2(char *, int, struct sockaddr_in *);
int parse_list_reply(char *rbuf, int len);
int ftp_get_size(struct sockaddr_in *sin, char *url, int type);

void http_head_req(struct request *req)
{
	struct sockaddr_in sin;
	struct hostent *he;
	int sd;
	char *sbuf;
	char *rbuf;
	char *tok;
	char *s;
	int clength;
	char connect_host[1024];
	int connect_host_port = 80;
	int nret = 0;

	if (strlen(http_proxyhost) > 0)  {
		strncpy(connect_host, http_proxyhost, 1023);
		connect_host_port = http_proxyport <= 0 ? 80 : http_proxyport;
	} else {
		strncpy(connect_host, req->host, 1023);
		connect_host_port = req->port;
	}

	sbuf = (char *)calloc(HEADREQSIZ + strlen(req->url), sizeof(char));
	rbuf = (char *)calloc(HEADREQSIZ, sizeof(char));

	if ((he = gethostbyname(connect_host)) == NULL) {
		Log("Error: Cannot resolve hostname for %s: %s", 
				req->host,
				hstrerror(h_errno));
		exit(1);
	}
	strncpy(req->ip, inet_ntoa(*(struct in_addr *)he->h_addr), MAXIPSIZ-1);
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr =inet_addr(req->ip);
	sin.sin_port = htons(connect_host_port);
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		Log("Socket creation failed for Head Request: %s", strerror(errno));
		exit(1);
	}
	if ((connect(sd, (const struct sockaddr *)&sin, sizeof(sin))) == -1) {
		Log("Connection failed for Head Request: %s", strerror(errno));
		exit(1);
	}
	Log("Head-Request Connection established");
	
	if(strlen(http_proxyhost) > 0) {
		char tmp[MAXBUFSIZ];
		snprintf(tmp, MAXBUFSIZ-1, "http://%s:%d%s", req->host, req->port, req->url);
		snprintf(sbuf, HEADREQSIZ-1, HEADREQ, tmp, req->host, PROGVERSION);
	} else {
		snprintf(sbuf, HEADREQSIZ-1, HEADREQ, req->url, req->host, PROGVERSION);
	}

	if ((send(sd, sbuf, strlen(sbuf), 0)) == -1) {
		Log("send failed for Head Request: %s", strerror(errno));
		exit(1);
	}
	if ((nret = recv(sd, rbuf, HEADREQSIZ, 0)) == -1) {
		Log("recv failed for Head Request: %s", strerror(errno));
		exit(1);
	}
	rbuf[nret] = '\0';
	handleHttpRetcode(rbuf);
	tok = strtok(rbuf, "\r\n");
        if ((strstr(tok, "HTTP/1.1 200")) != NULL) {
                while ((tok = strtok(NULL, "\r\n")) != NULL) {
                        if ((strstr(tok, "Content-Length")) != NULL) {
                                s = (tok + strlen("Content-Length: "));
                                clength = atoi(s);
				req->clength = clength;
                        }
                }
        }
	free(sbuf);
	free(rbuf);
}

int ftp_head_req(struct request *req, int *head_sd)
{
	struct sockaddr_in sin, sin_dc;
	struct hostent *he;
	int sd;
	char *sbuf;
	char *rbuf;
	int resume, ret;


	sbuf = (char *)calloc(MAXBUFSIZ + strlen(req->url), sizeof(char));
	rbuf = (char *)calloc(MAXBUFSIZ + 8, sizeof(char));

	resume = 0;

	if ((he = gethostbyname(req->host)) == NULL) {
		Log("Error: Cannot resolve hostname for %s: %s",
				req->host,
				hstrerror(h_errno));
		exit(1);
	}
	strncpy(req->ip, inet_ntoa(*(struct in_addr *)he->h_addr), MAXIPSIZ-1);
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr =inet_addr(req->ip);
	printf(req->ip);
	sin.sin_port = htons(req->port);
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		Log("Socket creation failed for Head Request: %s", strerror(errno));
		exit(1);
	}
	if ((connect(sd, (const struct sockaddr *)&sin, sizeof(sin))) == -1) {
		Log("Connection failed for Head Request: %s", strerror(errno));
		exit(1);
	}

	/* Should receive manually, cause some servers may not accept any connections!	*/
	if(recv_reply(sd, rbuf, MAXBUFSIZ, 0) == -1) {
		Log("recv failed: %s", strerror(errno));
		fprintf(stderr, "Server > %s", rbuf);
		exit(1);
	}
	else if(rbuf[0] != '2' && rbuf[0] != '1' && rbuf[0] != '3') {
		Log("Seems like the server isn't accepting connections now, bailing out...");
		fprintf(stderr, "Server > %s", rbuf);
		exit(1);
	}

	fprintf(stderr, "Server > %s", rbuf);

	rbuf[MAXBUFSIZ -2] = '\0';
	if(rbuf[0] != '2' && rbuf[0] != '1' && rbuf[0] != '3') {	/* '2' is the usual reply	*/
		handleFTPRetcode(rbuf);
		exit(1);
	}

	Log("Connection established to FTP server, logging in as %s", req->username);

	snprintf(sbuf, MAXBUFSIZ-1, "USER %s\r\n", req->username);
	printf("Aget > %s", sbuf);
	memset(rbuf, 0, MAXBUFSIZ);
	send(sd, sbuf, strlen(sbuf), 0);
	recv_reply(sd, rbuf, MAXBUFSIZ, 0);
	fprintf(stderr, "Server > %s\n", rbuf);
	if (strncmp(rbuf, "331", 3) != 0) {
		fprintf(stderr, "Server didnot like username, server said\n");
		fprintf(stderr, "Server > %s\n", rbuf);
		exit(1);
	}

	snprintf(sbuf, MAXBUFSIZ-1, "PASS %s\r\n", req->password);
	printf("Aget > PASS ***********\n");
	memset(rbuf, 0, MAXBUFSIZ);
	send(sd, sbuf, strlen(sbuf), 0);
	recv_reply(sd, rbuf, MAXBUFSIZ, 0);
	fprintf(stderr, "Server > %s\n", rbuf);
	if (strncmp(rbuf, "230", 3) != 0) {
		fprintf(stderr, "Server didnot accept password, server said\n");
		fprintf(stderr, "Server > %s\n", rbuf);
		exit(1);
	}
	Log("Successfully logged into FTP server, checking if server supports resume feature...");

	sprintf(sbuf, "REST 150\r\n");
	printf("Aget > %s", sbuf);
	memset(rbuf, 0, MAXBUFSIZ);
	send(sd, sbuf, strlen(sbuf), 0);
	recv_reply(sd, rbuf, MAXBUFSIZ, 0);
	fprintf(stderr, "Server > %s\n", rbuf);
	if (strncmp(rbuf, "350", 3) != 0) {
		fprintf(stderr, "Server doesn't accept resuming transfers. I'll use one thread, sorry.\n");
		fprintf(stderr, "Server > %s\n", rbuf);
		resume = 0;
		exit(1);
	}
	resume = 1;

	sprintf(sbuf, "SYST\r\n");
	printf("Aget > %s", sbuf);
	memset(rbuf, 0, MAXBUFSIZ);
	send(sd, sbuf, strlen(sbuf), 0);
	recv_reply(sd, rbuf, MAXBUFSIZ, 0);
	fprintf(stderr, "Server > %s\n", rbuf);
	if (strncmp(rbuf, "215", 3) != 0) {
		fprintf(stderr, "Server didnot return a reply to a SYST command.\n");
		fprintf(stderr, "Server > %s\n", rbuf);
		exit(1);
	}
	if (strstr(rbuf, "UNIX") != NULL)
		req->ftp_server_type = SERVER_TYPE_UNIX;
	else
		req->ftp_server_type = SERVER_TYPE_NT;

	sprintf(sbuf, "TYPE I\r\n");
	printf("Aget > %s", sbuf);
	memset(rbuf, 0, MAXBUFSIZ);
	send(sd, sbuf, strlen(sbuf), 0);
	recv_reply(sd, rbuf, MAXBUFSIZ, 0);
	fprintf(stderr, "Server > %s\n", rbuf);
	if (strncmp(rbuf, "200", 3) != 0) {
		fprintf(stderr, "Server didnot accept BINARY transfer.\n");
		fprintf(stderr, "Server > %s\n", rbuf);
	}

	sprintf(sbuf, "PASV\r\n");
	printf("Aget > %s", sbuf);
	memset(rbuf, 0, MAXBUFSIZ);
	send(sd, sbuf, strlen(sbuf), 0);
	recv_reply(sd, rbuf, MAXBUFSIZ, 0);
	fprintf(stderr, "Server > %s\n", rbuf);
	parse_pasv_reply2(rbuf, MAXBUFSIZ, &sin_dc);

	snprintf(sbuf, MAXBUFSIZ -1, "LIST %s\r\n", req->url + 1);
	printf("Aget > %s", sbuf);
	memset(rbuf, 0, MAXBUFSIZ);
	send(sd, sbuf, strlen(sbuf), 0);

	/* Get LISTing from Data Connection	*/
	if ((req->clength = ftp_get_size(&sin_dc, req->url + 1, req->ftp_server_type)) == -1) {
		fprintf(stderr, "Couldn't get SIZE for the URL!");
		exit(1);
	}
	ret = recv_reply(sd, rbuf, MAXBUFSIZ, 0);
	fprintf(stderr, "Server > %s\n", rbuf);
	
	Log("Content length: %d", req->clength);

/*	close(sd);		*/
	*head_sd = sd;

	free(sbuf);
	free(rbuf);

	return resume;
}


/* code by naveen n rao	*/
int parse_pasv_reply(char *rbuf, int len, struct sockaddr_in *sin)
{
	char *s;
	unsigned int addr[6] = {0, 0, 0, 0, 0, 0};
	char dc_addr[MAXIPSIZ];
	unsigned short dc_port;
	int i;
	int slen;
	

	s = strdup(rbuf);
	slen = strlen(rbuf);
	s += 4;

	i = 0;
	while(!isdigit(*s) && (i++ < slen))
		s++;

	for(i = 0; i < 6; i++) {
		while((i < 5 && *s != ',' ) || ( i == 5 && *s != ')'))
			addr[i] = (*(s++) - '0') + (addr[i] * 10);
		s++;
	}
	snprintf(dc_addr, MAXIPSIZ -1, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
	dc_port = (addr[4] << 8) + addr[5];

#ifdef DEBUG
	printf("PASV ADDR: %s, PORT: %d\n", dc_addr, dc_port);
#endif

	bzero((char *)sin, sizeof(struct sockaddr_in));
	sin->sin_family = AF_INET;
	sin->sin_port = htons(dc_port);
	sin->sin_addr.s_addr = inet_addr(dc_addr);

	return 0;
}

/* new */
int parse_pasv_reply2(char *rbuf, int len, struct sockaddr_in *sin)
{
        char *s = rbuf;
        char tmp[4];
        unsigned int addr[10] = {0, 0, 0, 0, 0, 0};
        char dc_addr[MAXIPSIZ];
        unsigned short dc_port;
        int i = 0, j = 0;

        for ( ; ((s - rbuf) < len) && (*s != '\0') && (*s != '\r') && (*s != '\n') && (*s != '(');)
                s++;
        s++;    /* pass '('     */
        for (i = 0 ; ((s - rbuf) < len) && (*s != '\0') && (*s != '\r') && (*s != '\n') && (i < 6); s++) {
                for (j = 0; (s - rbuf < len) && (*s != '\0') && (*s != '\r') && (*s != '\n') && (*s != ',') && (*s != ')') && (j < 3); s++, j++)
                        tmp[j] = *s;
                tmp[j] = '\0';
                addr[i] = atoi(tmp);
                i++;
        }
        if (addr[0] == 0)
                return -1;

        snprintf(dc_addr, MAXIPSIZ -1, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
        dc_port = (addr[4] << 8) + addr[5];

        bzero((char *)sin, sizeof(struct sockaddr_in));
        sin->sin_family = AF_INET;
        sin->sin_port = htons(dc_port);
        sin->sin_addr.s_addr = inet_addr(dc_addr);

        return 0;
}


/* by naceen n rao	*/
int ftp_get_size(struct sockaddr_in *sin, char *url, int type)
{
	int sd;
	char rbuf[MAXBUFSIZ];
	char perm[16];
	char attr[16];
	char uid[16];
	char gid[16];
	char size[16];
	char month[16];
	char day[16];
	char year[16];
	char file[255];

	memset(rbuf, 0x0, MAXBUFSIZ);
	memset(perm, 0x0, 16);
	memset(attr, 0x0, 16);
	memset(uid, 0x0, 16);
	memset(gid, 0x0, 16);
	memset(size, 0x0, 16);
	memset(month, 0x0, 16);
	memset(day, 0x0, 16);
	memset(year, 0x0, 16);
	memset(file, 0x0, 16);

	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		Log("Socket creation failed for FTP Data Connection: %s", strerror(errno));
		return -1;
	}
	if ((connect(sd, (const struct sockaddr *)sin, sizeof(struct sockaddr))) == -1) {
		Log("Connection failed FTP Data Connection: %s", strerror(errno));
		return -1;
	}

	memset(rbuf, 0, MAXBUFSIZ);
	recv(sd, rbuf, MAXBUFSIZ-1, 0);
	printf("Server > %s\n", rbuf);

	if (type == SERVER_TYPE_UNIX) {
		sscanf(rbuf, "%15s %15s %15s %15s %15s %15s %15s %15s %15s\r\n",
				perm,
				attr,
				uid,
				gid,
				size,
				month,
				day,
				year,
				file);
		printf("SIZE: %s\n", size);
	} else {
		/*
		-r-xr-xr-x   1 owner    group           10060 Apr 12  1999 Lmain.cpp
		*/
		sscanf(rbuf, "%15s %15s %15s %15s %15s %15s %15s %15s %15s\r\n",
				perm,
				attr,
				uid,
				gid,
				size,
				month,
				day,
				year,
				file);
		printf("SIZE: %s\n", size);
	}

	close(sd);
	return atoi(size);
}

/* by naceen n rao	*/
int parse_list_reply(char *rbuf, int len)
{
	char *tok;
	char *tokens[100];

	if ((tok = strtok_r(rbuf, "\r\n", tokens)) == NULL)
		return -1;

	fprintf(stderr, "Server > %s\n", tok);

	while ((tok = strtok_r(NULL, "\r\n", tokens)) != NULL)
		fprintf(stderr, "Server > %s\n", tok);

	return 0;
}
