#!/bin/bash

set -e

DIR=`dirname $0`
cd $DIR
cd ..
BUG_ROOT=`pwd`
if [ ! -d "$BUG_ROOT" ]; then
    echo "ERROR: unable to find bug3 directory"
    exit 1
fi;

SRC_DIR=$BUG_ROOT/src
if [ ! -d "$SRC_DIR" ]; then
    echo "ERROR: unable to find bug3 src directory"
    exit 1
fi;

JS_ZIP=$SRC_DIR/js-1.8.0-rc1.tar.gz
if [ ! -e "$JS_ZIP" ]; then
    echo "ERROR: unable to find zipped javascript interpreter source"
    exit 1
fi;

NSPR_ZIP=$SRC_DIR/nspr-4.7.3.tar.gz
if [ ! -e "$NSPR_ZIP" ]; then
    echo "ERROR: unable to find zipped NSPR source"
    exit 1
fi;

TEST_SRC=$SRC_DIR/test.c
if [ ! -e "$TEST_SRC" ]; then
    echo "ERROR: unable to find test harness $TEST_SRC"
    exit 1
fi;

PATCH1=$SRC_DIR/patches/476934-1.patch
if [ ! -e "$PATCH1" ]; then
    echo "ERROR: unable to find test harness $PATCH1"
    exit 1
fi;

PATCH2=$SRC_DIR/patches/467441-2.patch
if [ ! -e "$PATCH2" ]; then
    echo "ERROR: unable to find test harness $PATCH2"
    exit 1
fi;

PATCH3=$SRC_DIR/patches/477021-3.patch
if [ ! -e "$PATCH3" ]; then
    echo "ERROR: unable to find test harness $PATCH3"
    exit 1
fi;

PATCH4=$SRC_DIR/patches/478336-4.patch
if [ ! -e "$PATCH4" ]; then
    echo "ERROR: unable to find test harness $PATCH4"
    exit 1
fi;

NSPR_SRC=$SRC_DIR/nspr-4.7.3
NSPR_TARGET=$NSPR_SRC/target
JS_SRC=$SRC_DIR/js
JS_SRC_SRC=$SRC_DIR/js/src

BIN_DIR=$BUG_ROOT/bin
mkdir -p $BIN_DIR
if [ ! -d "$BIN_DIR" ]; then
    echo "ERROR: Unable to Create binary directory $BIN_DIR"
    exit 1
fi;

INSTALL_DIR=$BIN_DIR/install
mkdir -p $INSTALL_DIR
if [ ! -d "$INSTALL_DIR" ]; then
    echo "ERROR: Unable to Create installation directory $INSTALL_DIR"
    exit 1
fi;

BIN_DIR_README=$BIN_DIR/run

