#!/bin/bash

# Script to automate bug 3

function usage {
    echo "Usage: $0 [command modifier]"
    echo
    echo "Runs the benchmark.  If a command modifier is given, " 
    echo "then runs the benchmark under the command. Returns 0 if "
    echo "the bug is not found and 1 if the bug is found"
    echo
    echo "e.g. $0 \"pin --\" will result in the benchmark being run "
    echo "under pin (with an empty tool)"
    echo
}

if [ "$1" == "-h" ] || 
    [ "$1" == "--h" ] || 
    [ "$1" == "-help" ] || 
    [ "$1" == "--help" ]; 
then
    usage
    exit 0
fi

RUN_DIR=`pwd`

DIR=`dirname $0`
cd $DIR
SCRIPT_DIR=`pwd`

COMMON_SH=$SCRIPT_DIR/../../scripts/common.sh
if [ ! -e "$COMMON_SH" ]; then
    echo "ERROR: no common functions script"
    exit 0
fi

source $COMMON_SH


EXECUTABLE=$SCRIPT_DIR/../bin/test-js
if [ ! -e "$EXECUTABLE" ]; then
    echo "ERROR: Executable not found at $EXECUTABLE"
    exit 1
fi
EXECUTION_LOG=/tmp/spidermonkey2-log
TEST_CALL="{ $1 $EXECUTABLE; } & "


set -u

cd $RUN_DIR

function setup {
    rm -f $EXECUTION_LOG
    export LD_LIBRARY_PATH=$SCRIPT_DIR/../src/js/src/Linux_All_DBG.OBJ 
    return 0
}

function runtest {
    echo "[$0] Calling: $TEST_CALL"
    eval $TEST_CALL
    PID=$!
    wait $PID
    return $?
}

function cleanup {
    return 0
}

setup
runtest
RET=$?
cleanup

echo "[$0] Return value is $RET"
exit $RET
