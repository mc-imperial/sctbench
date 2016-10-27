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

MY_SRC=$TEST_SRC 

function usage {
    echo "$0 [default/sleep]"
    echo 
    echo "default builds the standard buggy build"
    echo
    echo "sleep builds nspr with added sleeps to make the bug more likely"
    echo
}

case "$1" in
    "sleep")
    MY_SRC=$TEST_SLEEP_SRC
    ;;
    
    "")
    usage
    exit 0
    ;;
    
esac

OUTNAME=test-rw

echo
echo "Building Test Harness..."
gcc -o $NSPR_TARGET/rwtest.o -c -fvisibility=hidden -Wall -pthread -g -fno-inline -fPIC  -UNDEBUG -DDEBUG_najalber  -DDEBUG=1 -DHAVE_VISIBILITY_HIDDEN_ATTRIBUTE=1 -DHAVE_VISIBILITY_PRAGMA=1 -DXP_UNIX=1 -D_GNU_SOURCE=1 -DHAVE_FCNTL_FILE_LOCKING=1 -DLINUX=1 -Di386=1 -DHAVE_LCHOWN=1 -DHAVE_STRERROR=1 -D_REENTRANT=1  -DFORCE_PR_LOG -D_PR_PTHREADS -UHAVE_CVAR_BUILT_ON_SEM -I$NSPR_DIST/include/nspr -I$NSPR_SRC/mozilla/nsprpub/pr/include -I$NSPR_SRC/mozilla/nsprpub/pr/include/private $MY_SRC
gcc $NSPR_TARGET/rwtest.o -Xlinker -rpath $NSPR_DIST/lib -L$NSPR_DIST/lib -lplc4 -L$NSPR_DIST/lib -lnspr4 -lpthread -o $BIN_DIR/$OUTNAME

EXECUTE_CMD="$BIN_DIR/$OUTNAME"

echo "Test Harness Built!"
echo "To Execute, you must specify the shared library path:"
echo "  $EXECUTE_CMD"
echo "$EXECUTE_CMD" &> $BIN_DIR_README

