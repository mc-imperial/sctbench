#!/bin/bash

set -e
DIR=`dirname $0`
COMMON_SH=$DIR/common.sh
if [ ! -e "$COMMON_SH" ]; then
    echo "ERROR: unable to find common.sh at $COMMON_SH"
    exit 1
fi

source $COMMON_SH

function usage {
    echo "$0 [default/sleep]"
    echo 
    echo "default builds the standard buggy build"
    echo
    echo "sleep builds nspr with added sleeps to make the bug more likely"
    echo
}

BUILD="default"

case "$1" in
    "sleep")
    BUILD="sleep"
    ;;
    
    "")
    usage
    exit 0
    ;;
    
esac



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

if [ "$BUILD" == "sleep" ]; then
    if [ ! -e "$NSPR_ORIGINAL_PTSYNCH" ]; then
        echo "ERROR: can't find original ptsynch.c at $NSPR_ORIGINAL_PTSYNCH"
        exit 1
    fi
    if [ ! -e "$NSPR_ORIGINAL_PTTHREAD" ]; then
        echo "ERROR: can't find original ptthread.c at "$
        echo "$NSPR_ORIGINAL_PTTHREAD"
        exit 1
    fi
    cp $NSPR_SLEEP_PTSYNCH $NSPR_ORIGINAL_PTSYNCH
    cp $NSPR_SLEEP_PTTHREAD $NSPR_ORIGINAL_PTTHREAD
    echo "NSPR built with sleeps to make bug more likely to occur"
fi


echo
echo "Starting Building NSPR..."
mkdir -p $NSPR_TARGET
cd $NSPR_TARGET
../mozilla/nsprpub/configure
make
echo "NSPR installed to $NSPR_DIST"
