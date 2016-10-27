#!/bin/bash


if [ "$mode" != "race" ]
then
	jobinfo=$(qstat ${PBS_JOBID} -n)
	echo $jobinfo
	regex="[/]([0-9]*)"
	[[ $jobinfo =~ $regex ]]
	cpuid="${BASH_REMATCH[1]}"
	echo taskset -p -c $cpuid $BASHPID
	taskset -p -c $cpuid $BASHPID

fi

out=$(echo Child shell; taskset -c -p $BASHPID)

echo $out

#env=$1
#shift

#source $env

$ROOT/scripts/run_single.py $qsubargs

