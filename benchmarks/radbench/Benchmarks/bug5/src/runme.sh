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
EXECUTABLE=$BIN_DIR/test-cvar
if [ ! -e "$EXECUTABLE" ]; then
    echo "ERROR: Executable not found at $EXECUTABLE"
    exit 1
fi
TEST_CALL="$1 $EXECUTABLE"

cd $RUN_DIR

function waitUntilTimeout {
    POLLS=20
    COUNT=0
    while [ "$POLLS" -gt "$COUNT" ]; 
    do
        kill -0 $1 
        if [ "$?" != "0" ]; then
            wait $1
            return $?
        fi
        sleep .1
        let "COUNT = $COUNT + 1"
    done
    kill $1
    sleep .2
    kill $1
    sleep .2
    kill -9 $1
    return 1
}

function setup {
    return 0
}

function runtest {
    echo "Calling: $TEST_CALL"
    $TEST_CALL &
    waitUntilTimeout $! &> /dev/null
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
