#! /bin/bash
#PBS -l walltime=00:20:00
#PBS -l mem=32000mb
#PBS -l ncpus=4

env=$1
shift

source $env

$ROOT/benchmarks/run_single.py $*

