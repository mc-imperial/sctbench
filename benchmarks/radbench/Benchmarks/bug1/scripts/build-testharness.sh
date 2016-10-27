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

echo
echo "Building Test Harness..."
g++ $TEST_HARNESS -o $BIN_DIR/test-js -I $FF_SRC/obj-dbg/dist/include/ -L $FF_SRC/obj-dbg/dist/lib/ -lpthread -lmozjs
echo "Test Harness Built!"
echo "To Execute, you must specify the shared library path:"
echo "  LD_LIBRARY_PATH=$FF_SRC/obj-dbg/dist/lib/ $BIN_DIR/test-js"
echo "  LD_LIBRARY_PATH=$FF_SRC/obj-dbg/dist/lib/ $BIN_DIR/test-js" &> $BIN_DIR_README

