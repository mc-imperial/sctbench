#!/bin/bash

set -e

DIR=`dirname $0`
cd $DIR
cd ..
BUG_ROOT=`pwd`
if [ ! -d "$BUG_ROOT" ]; then
    echo "ERROR: unable to find bug1 directory"
    exit 1
fi;

SRC_DIR=$BUG_ROOT/src
if [ ! -d "$SRC_DIR" ]; then
    echo "ERROR: unable to find bug1 src directory"
    exit 1
fi;

FF_ZIP=$SRC_DIR/firefox-src-2009-09-09.tar.bz2
if [ ! -e "$FF_ZIP" ]; then
    echo "ERROR: unable to find zipped firefox source"
    exit 1
fi;

FF_CONFIG=$SRC_DIR/.mozconfig
if [ ! -e "$FF_CONFIG" ]; then
    echo "ERROR: unable to find firefox .mozconfig file"
    exit 1
fi;

MOD_SRC_DIR=$SRC_DIR/my-mods
if [ ! -e "$MOD_SRC_DIR" ]; then
    echo "ERROR: unable to find modified sources at $MOD_SRC_DIR"
    exit 1
fi;

JS_GC_SLEEP=$MOD_SRC_DIR/jsgc-sleeps-added.cpp
if [ ! -e "$JS_GC_SLEEP" ]; then
    echo "ERROR: unable to find modified source $JS_GC_SLEEP"
    exit 1
fi;

JS_CNTXT_SLEEP=$MOD_SRC_DIR/jscntxt-sleeps-added.cpp
if [ ! -e "$JS_CNTXT_SLEEP" ]; then
    echo "ERROR: unable to find modified sources at $JS_CNTXT_SLEEP"
    exit 1
fi;

JS_ATOM_SLEEP=$MOD_SRC_DIR/jsatom-sleeps-added.cpp
if [ ! -e "$JS_ATOM_SLEEP" ]; then
    echo "ERROR: unable to find modified sources at $JS_ATOM_SLEEP"
    exit 1
fi;

JS_SCRIPT_SLEEP=$MOD_SRC_DIR/jsscript-sleeps-added.cpp
if [ ! -e "$JS_SCRIPT_SLEEP" ]; then
    echo "ERROR: unable to find modified sources at $JS_SCRIPT_SLEEP"
    exit 1
fi;

JS_GC_DEFAULT=$MOD_SRC_DIR/jsgc.cpp
if [ ! -e "$JS_GC_DEFAULT" ]; then
    echo "ERROR: unable to find modified sources at $JS_GC_DEFAULT"
    exit 1
fi;

JS_CNTXT_DEFAULT=$MOD_SRC_DIR/jscntxt.cpp
if [ ! -e "$JS_CNTXT_DEFAULT" ]; then
    echo "ERROR: unable to find modified sources at $JS_CONTXT_DEFAULT"
    exit 1
fi;

JS_ATOM_DEFAULT=$MOD_SRC_DIR/jsatom.cpp
if [ ! -e "$JS_ATOM_DEFAULT" ]; then
    echo "ERROR: unable to find modified sources at $JS_ATOM_DEFAULT"
    exit 1
fi;

JS_SCRIPT_DEFAULT=$MOD_SRC_DIR/jsscript.cpp
if [ ! -e "$JS_SCRIPT_DEFAULT" ]; then
    echo "ERROR: unable to find modified sources at $JS_SCRIPT_DEFAULT"
    exit 1
fi;

TEST_HARNESS=$SRC_DIR/test-js.cpp
if [ ! -e "$TEST_HARNESS" ]; then
    echo "ERROR: unable to find firefox test harness"
    exit 1
fi;

FF_SRC=$SRC_DIR/firefox-src
MOZCONFIG_DEST=$FF_SRC/.mozconfig
MOZ_JS_GC=$FF_SRC/js/src/jsgc.cpp
MOZ_JS_CNTXT=$FF_SRC/js/src/jscntxt.cpp
MOZ_JS_ATOM=$FF_SRC/js/src/jsatom.cpp
MOZ_JS_SCRIPT=$FF_SRC/js/src/jsscript.cpp

BIN_DIR=$BUG_ROOT/bin
mkdir -p $BIN_DIR
if [ ! -d "$BIN_DIR" ]; then
    echo "ERROR: Unable to Create binary directory $BIN_DIR"
    exit 1
fi;

BIN_DIR_README=$BIN_DIR/run

