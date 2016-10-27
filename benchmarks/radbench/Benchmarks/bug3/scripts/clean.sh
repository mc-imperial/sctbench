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

echo "Removing Bin Dir..."
rm -rf $BIN_DIR
echo "Removing NSPR source..."
rm -rf $NSPR_SRC
echo "Removing JS source..."
rm -rf $JS_SRC

