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
    echo "$0 [sleep/default]"
    echo 
    echo "sleep will build firefox with sleep "
    echo "statments which cause the bug manifest more often"
    echo
    echo "default will build with no sleep statments"
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
    *)
    BUILD="default"
    ;;
esac

set -u

if [ ! -d "$FF_SRC" ]; then 
    echo
    echo "Unzipping Firefox Source..."
    cd $SRC_DIR
    tar xjf $FF_ZIP
    if [ ! -d "$FF_SRC" ]; then
        echo "ERROR: unzip failed to create $FF_SRC"
        exit 1
    fi;
fi


if [ ! -e "$MOZCONFIG_DEST" ]; then
    echo
    echo "Moving Build Configuration File into Source Directory..."
    cp $FF_CONFIG $MOZCONFIG_DEST
    if [ ! -e "$MOZCONFIG_DEST" ]; then
        echo "ERROR: unable to move build config file to $MOZCONFIG_DEST"
        exit 1
    fi
fi

if [ "$BUILD" == "default" ]; then
    echo "Creating Default Build..."
    cp $JS_CNTXT_DEFAULT $MOZ_JS_CNTXT
    cp $JS_GC_DEFAULT $MOZ_JS_GC
    cp $JS_ATOM_DEFAULT $MOZ_JS_ATOM
    cp $JS_SCRIPT_DEFAULT $MOZ_JS_SCRIPT
fi

if [ "$BUILD" == "sleep" ]; then
    echo "Creating Sleep Build..."
    cp $JS_CNTXT_SLEEP $MOZ_JS_CNTXT
    cp $JS_GC_SLEEP $MOZ_JS_GC
    cp $JS_ATOM_SLEEP $MOZ_JS_ATOM
    cp $JS_SCRIPT_SLEEP $MOZ_JS_SCRIPT
fi

echo
echo "Starting Building Firefox..."
cd $FF_SRC
make -f client.mk build
echo "Firefox binary can now be found in firefox-src/obj-dbg/dist/bin/"
