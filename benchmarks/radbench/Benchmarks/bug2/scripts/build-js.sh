#!/bin/bash

set -e
DIR=`dirname $0`
COMMON_SH=$DIR/common.sh
if [ ! -e "$COMMON_SH" ]; then
    echo "ERROR: unable to find common.sh at $COMMON_SH"
    exit 1
fi

source $COMMON_SH

set -u

if [ ! -d "$NSPR_SRC" ]; then 
    echo
    echo "Unzipping NSPR Source..."
    cd $SRC_DIR
    tar xzf $NSPR_ZIP
    if [ ! -d "$NSPR_SRC" ]; then
        echo "ERROR: unzip failed to create $NSPR_SRC"
        exit 1
    fi
fi

if [ ! -d "$JS_SRC" ]; then 
    echo
    echo "Unzipping JS Source..."
    cd $SRC_DIR
    tar xzf $JS_ZIP
    if [ ! -d "$JS_SRC" ]; then
        echo "ERROR: unzip failed to create $JS_SRC"
        exit 1
    fi
fi

echo
echo "Starting Building NSPR..."
mkdir -p $NSPR_TARGET
cd $NSPR_TARGET
../mozilla/nsprpub/configure
make
make install
echo "NSPR installed to $NSPR_INSTALL"

echo
echo "Starting JS build..."
cd $JS_SRC_SRC
PATH=${NSPR_INSTALL}/bin:${PATH} XCFLAGS="-I $NSPR_INSTALL/include/nspr $(nspr-config --cflags)" XLDFLAGS="$(nspr-config --libs)" JS_THREADSAFE=1 make -f Makefile.ref
echo "JS built"
