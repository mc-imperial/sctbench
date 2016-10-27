#!/bin/bash

# WARNING: do not use httpd on anywhere with a command line.
# This script will killall httpd and kill your benchmark if you
# are not careful 

function usage {
    echo "Usage: $0 [option] [iterations]" 
    echo
    echo "Runs bug 15 to get timing results."
    echo "Iterations is the number of runs."
    echo "option can be:"
    echo "    all - all configurations"
    echo "    native - no instrumentation"
    echo "    pin - just pin"
    echo "    shmem - pin with shared memory approximation"
    echo "    pinplay - TODO: implement"
    echo "    trace - run with lwtrace tool"
    echo "    profile - run with trace profiler tool"
    echo
}

if [ "$1" == "" ];
then
    usage
    exit 0
fi

if [ "$2" == "" ];
then
    usage
    exit 0
fi

set -u

BUG_NAME=bug15

OPTION=$1
ITERATIONS=$2

DIR=`dirname $0`
cd $DIR
BENCHMARK_SCRIPT_DIR=`pwd`
cd ..
BENCHMARK_ROOT_DIR=`pwd`

BUG_RUNME=$BENCHMARK_ROOT_DIR/$BUG_NAME/scripts/runme.sh
if [ ! -e $BUG_RUNME ]; then
    echo "ERROR: unable to discover bug runme at $BUG_RUNME"
    exit 1
fi

COMMON_SH=$BENCHMARK_ROOT_DIR/scripts/common.sh
if [ ! -e $COMMON_SH ]; then
    echo "ERROR: unable to discover common functions script at $COMMON_SH"
    exit 1
fi

SHMEM_TOOL=$BENCHMARK_ROOT_DIR/../Sources/bin/locker.so
if [ ! -e $SHMEM_TOOL ]; then
    echo "ERROR: unable to discover shared mem approx tool $SHMEM_TOOL"
    exit 1
fi

TRACE_TOOL=$BENCHMARK_ROOT_DIR/../Sources/bin/lwtrace.so
if [ ! -e $TRACE_TOOL ]; then
    echo "ERROR: unable to discover tracing tool $TRACE_TOOL"
    exit 1
fi

PROFILE_TOOL=$BENCHMARK_ROOT_DIR/../Sources/bin/profiler.so
if [ ! -e $PROFILE_TOOL ]; then
    echo "ERROR: unable to discover profiling tool $PROFILE_TOOL "
    exit 1
fi


EXPERIMENT_DIR=$BENCHMARK_ROOT_DIR/../Experiments/
if [ ! -d $EXPERIMENT_DIR ]; then
    echo "ERROR: unable to experiment directory $EXPERIMENT_DIR"
    exit 1
fi
BUG_OUT_DIR=$EXPERIMENT_DIR/`date +%d-%m-%Y`/$BUG_NAME/
echo 
echo "Outputting results to $BUG_OUT_DIR"
echo 

mkdir -p $BUG_OUT_DIR
cd $BUG_OUT_DIR

function benchmarkOne {
    rm -f $SHARED_IP_DIR/*
    eval "{ $1 \"$BENCH_FLAG_PRE $2 $BENCH_FLAG_POST\"; } &"
    ASYNC=$!
    echo "Timeout in $MAX_TIME_SEC"
    waitUntilTimeout $ASYNC $MAX_TIME_SEC
    cat $SHARED_IP_DIR/* > $OUTDIR/run-$3.ips
    mkdir $OUTDIR/run-$3-tracedir
    mv /dev/shm/run-$3.trace* $OUTDIR/run-$3-tracedir/
    return $?
}

source $COMMON_SH

function native {
    BENCH_FLAG_PRE="/usr/bin/time -v -o"
    BENCH_FLAG_POST=""
    OUTDIR=$BUG_OUT_DIR/native
    MAX_TIME_SEC=30
    benchmarkIterations $BUG_NAME
}

function pin {
    BENCH_FLAG_PRE="/usr/bin/time -v -o"
    BENCH_FLAG_POST="pin -follow_execv --"
    OUTDIR=$BUG_OUT_DIR/pin
    MAX_TIME_SEC=100
    benchmarkIterations $BUG_NAME
}

SHARED_IP_DIR=/tmp/mysql-$BUG_NAME-sharedip
mkdir -p $SHARED_IP_DIR
SHARED_IP_OUT=$SHARED_IP_DIR/ip

function shmem {
    BENCH_FLAG_PRE="/usr/bin/time -v -o"
    BENCH_FLAG_POST="pin -follow_execv -t $SHMEM_TOOL -o $SHARED_IP_OUT -lock 0x8d108 -unlock 0x8cbb8 --"
    OUTDIR=$BUG_OUT_DIR/shmem
    MAX_TIME_SEC=500
    benchmarkIterations $BUG_NAME
}

function trace {
    SHARED_IPS=$BENCHMARK_ROOT_DIR/$BUG_NAME/exp-data/shared-ips
    if [ ! -e $SHARED_IPS ]; then
        echo "ERROR: shared IPs file missing at $SHARED_IPS"
        exit 1
    fi
    BENCH_FLAG_PRE="/usr/bin/time -v -o"
    #BENCH_FLAG_POST='pin -follow_execv -t $TRACE_TOOL -i $SHARED_IPS -o $OUTDIR/run-$3.trace --'
    rm -f /dev/shm/*trace*
    BENCH_FLAG_POST='pin -follow_execv -t $TRACE_TOOL -i $SHARED_IPS -o /dev/shm/run-$3.trace --'
    # BENCH_FLAG_POST='pin -follow_execv -t $TRACE_TOOL -i $SHARED_IPS --'
    OUTDIR=$BUG_OUT_DIR/trace
    MAX_TIME_SEC=500
    benchmarkIterations $BUG_NAME
    cp $SHARED_IPS $OUTDIR
}

function profile {
    SHARED_IPS=$BENCHMARK_ROOT_DIR/$BUG_NAME/exp-data/shared-ips
    if [ ! -e $SHARED_IPS ]; then
        echo "ERROR: shared IPs file missing at $SHARED_IPS"
        exit 1
    fi
    BENCH_FLAG_PRE="/usr/bin/time -v -o"
    BENCH_FLAG_POST='pin -follow_execv -t $PROFILE_TOOL -i $SHARED_IPS -o $OUTDIR/run-$3.profile --'
    OUTDIR=$BUG_OUT_DIR/profile
    MAX_TIME_SEC=500
    benchmarkIterations $BUG_NAME
    cp $SHARED_IPS $OUTDIR
}

function pinplay {
    echo "TODO--implement me!"
}

function all {
    native
    pin
    shmem
    pinplay
}

killAllWithProcName mysqld 10

eval $OPTION

