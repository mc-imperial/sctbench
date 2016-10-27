#!/bin/bash


if [ "$1" == "" ]; then
    echo "usage: $0 [path/to/experiment/dir]"
    echo
    echo "runs the summarizer on each trace"
    exit 0
fi


SUMMARIZER=/home/najalber/mt-testing/Sources/scripts/pldi/src/rtn_interrupted
if [ ! -e $SUMMARIZER ]; then
    echo "ERROR: cannot find summarizer $SUMMARIZER"
    exit 1
fi


i=0

while [ "$i" -lt "150" ]; do
    echo "****SUMMARIZING RUN $i***"
    TRACE_DIR_NAME=$1/run-$i-tracedir/
    if [ ! -d $TRACE_DIR_NAME ]; then
        let "i = i + 1"
        continue 
    fi
    OUT_NAME=$1/run-$i-summary
    cmd="{ time $SUMMARIZER $TRACE_DIR_NAME/*; } 2>&1 | tee $OUT_NAME"
    echo "[$0] Executing $cmd"
    eval $cmd
    let "i = i + 1"
done


echo "summarized $i traces"
