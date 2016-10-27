#!/bin/bash

# Common functions in the autobenchmarking scripts

# waitUntilTimeoutChrome(int pid, int timeout_sec)
# returns:      0 on no error
#               1 on error in benchmark
#               2 on timeout
function waitUntilTimeout {
    if [ "$1" == "" ]; then
        echo "ERROR: no process id specified"
        exit 2
    fi
    
    if [ "$2" == "" ]; then
        echo "ERROR: no timeout value specified"
        exit 2
    fi

    TARGET_PID=$1

    MAX_POLLS=0
    let "MAX_POLLS = $2 * 10"

    SPINS=0
    while [ "$MAX_POLLS" -gt "$SPINS" ]; 
    do
        kill -0 $TARGET_PID  &> /dev/null
        CHROME_STILL_EXISTS=$?
        if [ "$CHROME_STILL_EXISTS" != "0" ]; then
            wait $TARGET_PID
            return $?
        fi
        sleep .1
        let "SPINS = $SPINS + 1"
    done

    # Oops--timeout
    kill -9 $1 &> /dev/null
    return 2
}

# Optional second argument which is number of spins before kill -9

function killAllWithProcName {
    if [ "$1" == "" ]; then
        echo "ERROR: need name argument to killall with name"
        exit 1
    fi
    MAX_SPINS=0
    set +u
    if [ "$2" != "" ]; then
        MAX_SPINS=$2
    fi
    set -u
    KILL_CALL="killall $1 &> /dev/null"
    KILL_COUNT=0
    eval $KILL_CALL
    KILL_RET=$?
    while [ "$KILL_RET" == "0" ]; do
        eval $KILL_CALL
        KILL_RET=$?
        let "KILL_COUNT = $KILL_COUNT + 1"
        sleep .1
        if [ $MAX_SPINS -gt 0 ] && [ $KILL_COUNT -gt $MAX_SPINS ]; then
            echo "TIMEOUT: Begin kill -9 $1"
            KILL_CALL="killall -9 $1 &> /dev/null"
        fi

    done
}

#Warning will kill timing functions etc (if the passed name is on the cmd line)
function killAllCmdContainingName {
    if [ "$1" == "" ]; then
        echo "ERROR: need name argument to killall with name"
        exit 1
    fi
    MAX_SPINS=0
    set +u
    if [ "$2" != "" ]; then
        MAX_SPINS=$2
    fi
    set -u
    PGREP_CALL="pgrep -f $1 | xargs kill &> /dev/null"
    PGREP_COUNT=0
    eval $PGREP_CALL
    PGREP_RET=$?
    while [ "$PGREP_RET" == "0" ]; do
        eval $PGREP_CALL
        PGREP_RET=$?
        let "PGREP_COUNT = $PGREP_COUNT + 1"
        if [ $MAX_SPINS -gt 0 ] && [ $PGREP_COUNT -gt $MAX_SPINS ]; then
            echo "TIMEOUT: Begin kill -9 $1"
            PGREP_CALL="pgrep -f $1 | xargs kill -9 &> /dev/null"
        fi

    done
}

function benchmarkIterations {
    PROGRAM_NAME=$1
    echo
    echo "Benchmarking $PROGRAM_NAME..."

    mkdir -p $OUTDIR
    
    #echo "Warmup run:" 
    #echo 
    #benchmarkOne $BUG_RUNME /tmp/time "warmup" 2>&1 | tee $OUTDIR/run-warmup.log
    #cat /tmp/time
    #cat /tmp/time >> $OUTDIR/run-warmup.log
    #echo 
    #echo "."
    #echo

    COUNT=0
    while [ "$COUNT" -lt "$ITERATIONS" ]; do
        OUTFILE=$OUTDIR/run-$COUNT
        benchmarkOne $BUG_RUNME $OUTFILE $COUNT 2>&1 | tee $OUTDIR/run-$COUNT.log
        cat $OUTFILE
        RET_VAL=$?
        if [ "$RET_VAL" != "2" ]; then
            let "COUNT = $COUNT + 1"
            echo 
            echo "."
            echo
        else
            echo
            echo "Benchmark error--retrying run"
            echo
        fi
    done
}


function serverWaitUntilAvailable {
    if [ "$1" == "" ]; then
        echo "ERROR: need port argument to server wait"
        exit 1
    fi
    WGET_CALL="wget -O /dev/null -o /dev/null http://localhost:$1"
    eval $WGET_CALL
    RET=$?
    while [ "$RET" != "0" ]; do
        sleep .2
        eval $WGET_CALL
        RET=$?
    done
}

function mysqlWaitUntilAvailable {
    MYSQL_CALL="$MYSQL_CLIENT -e \"show databases;\" &> /dev/null"
    eval $MYSQL_CALL
    RET=$?
    while [ "$RET" != "0" ]; do
        sleep .2
        eval $MYSQL_CALL
        RET=$?
    done
}

function chromeKillAndWait {
    killall chrome &> /dev/null
    KILL_RET=$?
    while [ "$KILL_RET" == "0" ]; do
        sleep .2
        killall chrome &> /dev/null
        KILL_RET=$?
    done
}

function chromeWait {
    KILL_RET=0
    while [ "$KILL_RET" == "0" ]; do
        sleep .2
        killall -0 chrome &> /dev/null
        KILL_RET=$?
    done
}

function chromeQuitAndWait {
    KILL_RET=0
    xdotool key shift+ctrl+q
    killall -0 chrome &> /dev/null
    while [ "$KILL_RET" == "0" ]; do
        sleep .5
        xdotool key shift+ctrl+q
        killall -0 chrome &> /dev/null
        KILL_RET=$?
    done

}

function killXvfb {
    killAllWithProcName "Xvfb"
}

function startXvfbOnPort {
    if [ "$1" == "" ]; then
        echo "ERROR: must specify port to start xvfb on"
        exit 1
    fi;
    killXvfb
    X_CALL="Xvfb :$1 -ac -screen 0 1024x768x24 &"
    eval $X_CALL
    sleep 1
}
