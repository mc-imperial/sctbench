#!/bin/bash

set -e
set -u
DIR=`dirname $0`
COMMON_SH=$DIR/common.sh
if [ ! -e "$COMMON_SH" ]; then
    echo "ERROR: unable to find common.sh at $COMMON_SH"
    exit 1
fi

source $COMMON_SH

OUTNAME=test-time

echo
echo "Building Test Harness..."
#gcc -m32 -o $NSPR_TARGET/threads.o -c -fvisibility=hidden -pipe -Wall -pthread -g -fno-inline -fPIC  -UNDEBUG -DDEBUG_najalber  -DDEBUG=1 -DHAVE_VISIBILITY_HIDDEN_ATTRIBUTE=1 -DHAVE_VISIBILITY_PRAGMA=1 -DXP_UNIX=1 -D_GNU_SOURCE=1 -DHAVE_FCNTL_FILE_LOCKING=1 -DLINUX=1 -Di386=1 -DHAVE_LCHOWN=1 -DHAVE_STRERROR=1 -D_REENTRANT=1  -DFORCE_PR_LOG -D_PR_PTHREADS -UHAVE_CVAR_BUILT_ON_SEM -I../../dist/include/nspr -I../../../mozilla/nsprpub/pr/include -I../../../mozilla/nsprpub/pr/include/private  ../../../mozilla/nsprpub/pr/tests/threads.c
#gcc -m32  threads.o -Xlinker -rpath /home/najalber/bugs/bug4/src/nspr-4.6.4/target/pr/tests/../../dist/lib -L../../dist/lib -lplc4 -L../../dist/lib -lnspr4 -lpthread -o threads

gcc -o $BIN_DIR/$OUTNAME -fvisibility=hidden -pipe -Wall -pthread -g -fno-inline -fPIC  -UNDEBUG -DDEBUG_najalber  -DDEBUG=1 -DHAVE_VISIBILITY_HIDDEN_ATTRIBUTE=1 -DHAVE_VISIBILITY_PRAGMA=1 -DXP_UNIX=1 -D_GNU_SOURCE=1 -DHAVE_FCNTL_FILE_LOCKING=1 -DLINUX=1 -Di386=1 -DHAVE_LCHOWN=1 -DHAVE_STRERROR=1 -D_REENTRANT=1  -DFORCE_PR_LOG -D_PR_PTHREADS -UHAVE_CVAR_BUILT_ON_SEM -I$NSPR_DIST/include/nspr -I$NSPR_SRC/mozilla/nsprpub/pr/include -I$NSPR_SRC/mozilla/nsprpub/pr/include/private $TEST_SRC $NSPR_DIST/lib/libnspr4.a $NSPR_DIST/lib/libplc4.a $NSPR_DIST/lib/libplds4.a -lpthread -ldl
#gcc $NSPR_TARGET/threads.o -Xlinker -rpath $NSPR_DIST/lib/libnspr4.a $NSPR_DIST/lib/libplc4.a $NSPR_DIST/lib/libplds4.a -lpthread -o $BIN_DIR/$OUTNAME

EXECUTE_CMD="$BIN_DIR/$OUTNAME"

echo "Test Harness Built!"
echo "To Execute, you must specify the shared library path:"
echo "  $EXECUTE_CMD"
echo "$EXECUTE_CMD" &> $BIN_DIR_README

