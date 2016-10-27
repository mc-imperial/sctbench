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
OUTNAME=test-ctxt

echo
echo "Building Test Harness..."
g++ -g $TEST_SRC -o $BIN_DIR/$OUTNAME -I $JS_SRC_SRC -I $JS_SRC_SRC/*.OBJ/ -I $NSPR_INSTALL/include/nspr/ -L $NSPR_INSTALL/lib/ -L $JS_SRC_SRC/*.OBJ/ -lpthread -ljs
echo "Test Harness Built!"
echo "To Execute, you must specify the shared library path:"
echo "  LD_LIBRARY_PATH=$OBJ_OUTPUT $BIN_DIR/$OUTNAME"
echo "LD_LIBRARY_PATH=$OBJ_OUTPUT $BIN_DIR/$OUTNAME" &> $BIN_DIR_README

