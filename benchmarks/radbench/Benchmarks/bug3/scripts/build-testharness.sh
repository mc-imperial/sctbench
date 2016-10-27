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


OBJ_OUTPUT=`ls $JS_SRC_SRC | grep OBJ`
if [ "$OBJ_OUTPUT" == "" ]; then
    echo "ERROR: unable to find JS engine compile"
    exit 1
fi
OBJ_OUTPUT=$JS_SRC_SRC/$OBJ_OUTPUT

echo
echo "Building Test Harness..."
gcc -g -DXP_UNIX -DJS_THREADSAFE -I $JS_SRC_SRC -I $JS_SRC_SRC/*.OBJ/ $SRC_DIR/test.c -L $JS_SRC_SRC/*.OBJ/ -ljs -o $BIN_DIR/test-js
echo "Test Harness Built!"
echo "To Execute, you must specify the shared library path:"
echo "  LD_LIBRARY_PATH=$OBJ_OUTPUT $BIN_DIR/test-js"
echo "  LD_LIBRARY_PATH=$OBJ_OUTPUT $BIN_DIR/test-js" &> $BIN_DIR_README

