#ifndef LOADCONFIG_H
#define LOADCONFIG_H
#include <stdio.h>

#define TOKENS " \n"

enum {
        BUFSIZE = 1024, 
        KEYSIZE = 64,
        VALSIZE = 256
};

char ftpanonymoususer[VALSIZE];
char ftpanonymouspass[VALSIZE];
int preferredthread = -1;
char http_proxyhost[VALSIZE];
int http_proxyport = -1;
char http_proxyuser[VALSIZE];
char http_proxypass[VALSIZE];

void loadconfig(char *);
void readrc();
void removespaces(char *);
#endif
