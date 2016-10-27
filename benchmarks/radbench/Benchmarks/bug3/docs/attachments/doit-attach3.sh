#!/bin/bash -e

DIR=/tmp/bug478336

mkdir ${DIR}

# Download NSPR and build it, as per
# https://developer.mozilla.org/en/Checkout_and_Build_NSPR

cd ${DIR}
wget http://ftp.mozilla.org/pub/mozilla.org/nspr/releases/v4.7.3/src/nspr-4.7.3.tar.gz
gunzip -c nspr-4.7.3.tar.gz | tar xf -
cd nspr-4.7.3
mkdir target
cd target
../mozilla/nsprpub/configure --prefix=${DIR}
make
make install

# Download SM 1.8 RC1 and build it

cd ${DIR}
wget http://ftp.mozilla.org/pub/mozilla.org/js/js-1.8.0-rc1.tar.gz
gunzip -c js-1.8.0-rc1.tar.gz | tar xf -
cd js/src
wget -O- https://bug478336.bugzilla.mozilla.org/attachment.cgi?id=366581 | patch -p0
export PATH=${DIR}/bin:${PATH}
XCFLAGS="$(nspr-config --cflags)" XLDFLAGS="$(nspr-config --libs)" JS_THREADSAFE=1 make -f Makefile.ref
cat > test.c <<!
#include <stdlib.h>
#include <pthread.h>

#include "jsapi.h"

static JSRuntime *rt;

#define THREADS 100

static void * testfunc(void *ignored) {

int i;
for (i = 0; i < 100; ++i) {
    JSContext *cx = JS_NewContext(rt, 0x1000);
    if (cx) {
        JS_BeginRequest(cx);
        JS_DestroyContext(cx);
    }
}

    return NULL;
}

int main(void) {

    rt = JS_NewRuntime(0x100000);
    if (rt == NULL)
        return 1;

    /* Uncommenting this to guarantee there's always at least
     * one context in the runtime prevents this problem. */
//  JSContext *cx = JS_NewContext(rt, 0x1000);

    int i;
    pthread_t thread[THREADS];
    for (i = 0; i < THREADS; i++) {
        pthread_create(&thread[i], NULL, testfunc, NULL);
    }

    for (i = 0; i < THREADS; i++) {
        pthread_join(thread[i], NULL);
    }

    return 0;
}
!
gcc -g -DXP_UNIX -DJS_THREADSAFE -I . -I *.OBJ/ test.c -L *.OBJ/ -ljs
cd *.OBJ
LD_LIBRARY_PATH=. ../a.out

