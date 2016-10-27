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

JS_ZIP=$SRC_DIR/js-1.7.0.tar.gz
if [ ! -e "$JS_ZIP" ]; then
    echo "ERROR: unable to find zipped javascript interpreter source"
    exit 1
fi;

NSPR_ZIP=$SRC_DIR/nspr-4.6.7.tar.gz
if [ ! -e "$NSPR_ZIP" ]; then
    echo "ERROR: unable to find zipped NSPR source"
    exit 1
fi;

TEST_SRC=$SRC_DIR/test-ctxt.cpp
if [ ! -e "$TEST_SRC" ]; then
    echo "ERROR: unable to find test harness $TEST_SRC"
    exit 1
fi;

NSPR_SRC=$SRC_DIR/nspr-4.6.7
NSPR_TARGET=$NSPR_SRC/target
NSPR_INSTALL=$NSPR_TARGET/dist
JS_SRC=$SRC_DIR/js
JS_SRC_SRC=$SRC_DIR/js/src

BIN_DIR=$BUG_ROOT/bin
mkdir -p $BIN_DIR
if [ ! -d "$BIN_DIR" ]; then
    echo "ERROR: Unable to Create binary directory $BIN_DIR"
    exit 1
fi;



BIN_DIR_README=$BIN_DIR/run

