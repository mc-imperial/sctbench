#!/bin/bash

#script to automate experiments

function usage {
    echo "Usage: $0 [command modifier]"
    echo
    echo "Runs the benchmark.  If a command modifier is given, " 
    echo "then runs the benchmark under the command. Returns 0 if "
    echo "the bug is not found and 1 if the bug is found"
    echo
    echo "e.g. $0 \"pin --\" will result in the benchmark being run "
    echo "under pin (with an empty tool"
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
BIN_DIR=`pwd`
EXECUTABLE=$BIN_DIR/test-js
if [ ! -e "$EXECUTABLE" ]; then
    echo "ERROR: Executable not found at $EXECUTABLE"
    exit 1
fi
TEST_CALL="$1 $EXECUTABLE"

cd ../src/js/src/Linux_All_DBG.OBJ
LIBRARY_DIR=`pwd`
if [ ! -d "$LIBRARY_DIR" ]; then
    echo "ERROR: Library dir not foudn at $LIBRARY_DIR"
    exit 1
fi

cd $RUN_DIR

function setup {
    export LD_LIBRARY_PATH=$LIBRARY_DIR
}

function runtest {
    echo "Calling: $TEST_CALL"
    $TEST_CALL
    return $?
}

function cleanup {
    return 0
}

setup
runtest
RESULT=$?
cleanup

exit $RESULT
