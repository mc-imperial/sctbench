#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <pwd.h>

#include "loadrc.h"

extern int errno;

void loadconfig(char *cfgfile)  /* load aget rc file */
{
	FILE *fd;
	char buf[BUFSIZE];
	char keyword[KEYSIZE];
	char value[VALSIZE];
	char *cp1, *cp2;
	char *variables[] = { "Invalid",
		"ftpanonymoususer",
		"ftpanonymouspass",
		"http_proxyhost",
		"http_proxyuser",
		"http_proxypass",
		"preferredthread"
	};

	int i, key, line, keyword_nums = sizeof(variables)/sizeof(char *);
	int slen = 0;
	int j = 0;

	if ((fd = fopen(cfgfile, "r")) == NULL) {
		fprintf(stderr, "loadconfig: cannot open aget rc file %s, exiting...\n", cfgfile);
		exit(-1);
	}
	line = 0;
	i = 0;
	while ((fgets(buf, BUFSIZE, fd)) != NULL) {
		line++;
		if (buf[0] == '#') 
			continue;
		if ((slen = strlen(buf)) <= 1)
			continue;
		cp1 = buf;
		cp2 = keyword;
		while (isspace((int)*cp1) && (cp1 - buf < slen)) 
			cp1++;
		j = 0;
		while (isgraph((int)*cp1) && (*cp1 != '=') && (j++ < KEYSIZE -2) && (cp1 - buf < slen)) 
			*cp2++ = *cp1++;
		j = 0;
		*cp2 = '\0';
		cp2 = value;
		while ((*cp1 != '\0') && (*cp1 !='\n') && (*cp1 !='=') && (cp1 - buf < slen))
			cp1++;
		cp1++; 
		while (isspace((int)*cp1) && (cp1 - buf < slen))
			cp1++; 
		if (*cp1 == '"') 
			cp1++;
		while ((*cp1 != '\0') && (*cp1 !='\n') && (*cp1 !='"') && (j++ < VALSIZE -2) && (cp1 - buf < slen))
			*cp2++ = *cp1++;
		*cp2-- = '\0';
		if (keyword[0] =='\0' || value[0] =='\0')
			continue;
		key = 0;
		for (i = 0; i < keyword_nums; i++) {
			if ((strcmp(keyword, variables[i])) == 0) {
				key = i;
				break;
			}
		}


		switch(key) {
		case 0:
			fprintf(stderr, "Illegal Keyword in RC file %s: %s\n", cfgfile, keyword);
			break;
		case 1:
			strncpy(ftpanonymoususer, value, VALSIZE);
			break;
		case 2:
			strncpy(ftpanonymouspass, value, VALSIZE);
			break;
		case 3:
			strncpy(http_proxyhost, value, VALSIZE);
			break;
		case 4:
			strncpy(http_proxyuser, value, VALSIZE);
			break;
		case 5:
			strncpy(http_proxypass, value, VALSIZE);
			break;
		case 6:
			preferredthread = atoi(value);
			break;
		}
	}
	fclose(fd);
}


void createrc(char *f)
{
	FILE *fp;

	if ((fp = fopen(f, "w")) == NULL) {
		fprintf(stderr, "Aget: couldn't create agetrc file %s: %s\n", f, strerror(errno));
		return;
	}

	fprintf(fp, "ftpanonymoususer = ftp\n");
	fprintf(fp, "ftpanonymouspass = aget@enderunix.org\n");

	fclose(fp);
}


void readrc()
{
	struct passwd *pw;
	struct stat st;
	char agetrc[1024];
	char *tok;



        if ((pw = getpwuid(getuid())) == NULL) 
		return;

	snprintf(agetrc, 1020, "%s/.agetrc", pw->pw_dir);
	if ((stat(agetrc, &st)) == -1) {
		if (errno == ENOENT)
			createrc(agetrc);
		else {
			fprintf(stderr, "cannot read agetrc file %s: %s. Might be corrupt!\n", agetrc, strerror(errno));
			exit(1);
		}
	}
	else
		loadconfig(agetrc);

	if (strlen(http_proxyhost) > 0) {
		if ((tok = strtok(http_proxyhost, ":")) != NULL) {
			strncpy(http_proxyhost, tok, VALSIZE -1);
			tok = strtok(NULL, ":");
			if(tok != NULL) {
				http_proxyport = atoi(tok);
			} else {
				http_proxyport = 80;
			}
		}
	}


#ifdef DEBUG
	printf("ftpanon: %s\n", ftpanonymoususer);
	printf("ftpanonpa: %s\n", ftpanonymouspass);
	printf("ph: %s\n", http_proxyhost);
	printf("pu: %s\n", http_proxyuser);
	printf("ppass: %s\n", http_proxypass);
	printf("pport: %d\n", http_proxyport);
	printf("pt = %d\n", preferredthread);
#endif
}
