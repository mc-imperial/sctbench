#!/bin/bash

set -e

DIR=`dirname $0`
cd $DIR
cd ..
BUG_ROOT=`pwd`
if [ ! -d "$BUG_ROOT" ]; then
    echo "ERROR: unable to find bug2 directory"
    exit 1
fi;

SRC_DIR=$BUG_ROOT/src
if [ ! -d "$SRC_DIR" ]; then
    echo "ERROR: unable to find bug2 src directory"
    exit 1
fi;

NSPR_ZIP=$SRC_DIR/nspr-4.8.6.tar.gz
if [ ! -e "$NSPR_ZIP" ]; then
    echo "ERROR: unable to find zipped NSPR source"
    exit 1
fi;

TEST_SRC=$SRC_DIR/cvar.c
if [ ! -e "$TEST_SRC" ]; then
    echo "ERROR: unable to find test harness $TEST_SRC"
    exit 1
fi;

NSPR_SRC=$SRC_DIR/nspr-4.8.6
NSPR_TARGET=$NSPR_SRC/target
NSPR_DIST=$NSPR_TARGET/dist

NSPR_ORIGINAL_PTSYNCH=$NSPR_SRC/mozilla/nsprpub/pr/src/pthreads/ptsynch.c
NSPR_ORIGINAL_PTTHREAD=$NSPR_SRC/mozilla/nsprpub/pr/src/pthreads/ptthread.c

NSPR_SLEEP_PTSYNCH=$SRC_DIR/my-mods/ptsynch.c
if [ ! -e "$NSPR_SLEEP_PTSYNCH" ]; then
    echo "ERROR: unable to find NSPR ptsynch.c with added sleeps at"
    echo "    $NSPR_SLEEP_PTSYCN"
    exit 1
fi;

NSPR_SLEEP_PTTHREAD=$SRC_DIR/my-mods/ptthread.c
if [ ! -e "$NSPR_SLEEP_PTTHREAD" ]; then
    echo "ERROR: unable to find NSPR ptthread.c with added sleeps at"
    echo "    $NSPR_SLEEP_PTTHREAD"
    exit 1
fi;

BIN_DIR=$BUG_ROOT/bin
mkdir -p $BIN_DIR
if [ ! -d "$BIN_DIR" ]; then
    echo "ERROR: Unable to Create binary directory $BIN_DIR"
    exit 1
fi;


BIN_DIR_README=$BIN_DIR/run

