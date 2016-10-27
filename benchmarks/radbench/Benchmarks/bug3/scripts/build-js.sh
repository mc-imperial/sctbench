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
    echo "$0 [1.7/1.8/1.8-patch]"
    echo 
    echo "1.7 will build with SpiderMonkey 1.7.0 where bug does not show"
    echo
    echo "1.8 will build with SpiderMonkey 1.8.0-rc1 where bug *DOES* show"
    echo
    echo "1.8-patch will build with SpiderMonkey 1.8.0-rc1 but applies"
    echo "patches suggested by comment 21 of the bug report which should"
    echo "fix the bug"
    echo 
}

PATCH="apply"

case "$1" in
    "1.7")
    JS_ZIP=$SRC_DIR/js-1.7.0.tar.gz
    PATCH="original"
    ;;
    
    "")
    usage
    exit 0
    ;;
    
    "1.8-patch")
    JS_ZIP=$SRC_DIR/js-1.8.0-rc1.tar.gz
    PATCH="apply"
    ;;
    *)
    JS_ZIP=$SRC_DIR/js-1.8.0-rc1.tar.gz
    PATCH="original"
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
../mozilla/nsprpub/configure --prefix=$INSTALL_DIR
make
make install
echo "NSPR installed to $INSTALL_DIR"

if [ "$PATCH" == "apply" ]; then
    echo "Applying Patches to JS engine..."
    cd $JS_SRC_SRC
    patch -p0 < $PATCH1
    patch -p0 < $PATCH2
    patch -p0 < $PATCH3
    patch -p0 < $PATCH4
    echo "Patches Applied!"
fi


echo
echo "Starting JS build..."
cd $JS_SRC_SRC
PATH=${INSTALL_DIR}/bin:${PATH} XCFLAGS="$(nspr-config --cflags)" XLDFLAGS="$(nspr-config --libs)" JS_THREADSAFE=1 make -f Makefile.ref
echo "JS built"
