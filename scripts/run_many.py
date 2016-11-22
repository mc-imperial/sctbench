#! /usr/bin/env python

import os
import sys
import subprocess
from os import path
import errno
from shutil import copy2
import shutil
import threading
import re
import time
import signal

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

def commandLine ():
    from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
    
    cmdline = ArgumentParser(formatter_class=ArgumentDefaultsHelpFormatter)
    
    # The command-line parser and its options    
    cmdline.add_argument(action="store",
                         help="A list of tests to run.",
                         dest="testslist")
    
    # cmdline.add_argument("-wt",
    #                      "--walltime",
    #                       action="store",
    #                       type=str,
    #                       help="max wall time for each test",
    #                       default="09:00:00")
    
    # cmdline.add_argument("-mem",
    #                      "--mem",
    #                       action="store",
    #                       type=str,
    #                       help="max RAM needed",
    #                       default="800mb")
    
    # cmdline.add_argument("-ncpus",
    #                      "--ncpus",
    #                       action="store",
    #                       type=str,
    #                       help="#CPUs needed",
    #                       default="2")
    
    cmdline.add_argument("-mode",
                         "--mode",
                          action="store",
                          type=str,
                          help="race, pb, db, pct, random, dfs",
                          default="race")
    
    cmdline.add_argument("-max",
                         "--max",
                          action="store",
                          type=int,
                          help="",
                          default=5)
    
    cmdline.add_argument("-tl",
                         "--timelimit",
                          action="store",
                          type=int,
                          help="",
                          default=30)
    
    cmdline.add_argument("-nt",
                         "--numthreads",
                          action="store",
                          type=int,
                          help="",
                          default=2)

    cmdline.add_argument("-limit",
                         "--limit",
                          action="store",
                          type=int,
                          help="",
                          default=10000)

    cmdline.add_argument("-nj",
                         "--numjobs",
                          action="store",
                          type=int,
                          help="Splits random or pct into n jobs that can be run in parallel.",
                          default=10)

    cmdline.add_argument("-job",
                         "--job",
                          action="store",
                          type=int,
                          help="Which job to execute (see --numjobs).",
                          default=-1)
                          
    cmdline.add_argument("-bi",
                         "--benchmarkindex",
                          action="store",
                          type=int,
                          help="Which benchmark to execute.",
                          default=-1)
    
    return cmdline.parse_args()

def call_check(cmd, qsubargs):
    print qsubargs
    #qsubargs = " ".join(qsubargs)
    #os.environ["qsubargs"]=qsubargs
    subprocess.check_call(qsubargs)

if __name__ == "__main__":
    assert "PIN_HOME" in os.environ
    args = commandLine()
    
    script = "run_single.sh"
    script = path.join(os.environ["ROOT"], "scripts", script)
    
    envfile = path.join(os.environ["ROOT"], "cluster_env.sh")
    
    outdir = path.join(os.environ["ROOT"],"benchmarks","__results",time.strftime("%Y-%m-%d-%H-%M-%S"))
    mkdir_p(outdir)

    limit = int(args.limit)
    seed = 0
    numJobs = int(args.numjobs)

    limitPerJob = limit / numJobs
    counter = 0
    with open(args.testslist, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith('#') or len(line)==0:
                continue
            line = line.split()
            suite = line[0]
            test = line[1]
            maxThreads = line[2]
            maxSteps = line[3]
            if args.benchmarkindex == -1 or args.benchmarkindex == counter:
                # This was used to queue a job in our cluster. Not used in vagrant release.
                cmd = ["qsub",
                    "-l", "walltime="+args.walltime,
                    "-l", "mem="+args.mem,
                    "-l", "ncpus="+args.ncpus,
                    "-j", "oe",
                    "-N", (suite[0]+test)[0:15],
                    "-V",
                        #"--",
                        script]
                qsubargs = [script, suite, test, outdir,
                        str(args.timelimit), str(args.numthreads),
                        maxThreads, maxSteps]
                os.environ["mode"]=""

                if args.mode=="dfs":
                    if args.job == -1 or args.job == 0:
                        call_check(cmd, qsubargs + [str(0), str(0), str(limit), str(seed), "0", "dfs"])
                elif args.mode=="random":
                    for i in range(0, numJobs):
                        if args.job == -1 or args.job == i:
                            call_check(cmd, qsubargs + [str(0), str(0), str(limitPerJob), str(seed), str(i*limitPerJob), "random"])
                elif args.mode=="pct":
                    for i in range(0,numJobs):
                        if args.job == -1 or args.job == i:
                            call_check(cmd, qsubargs + [str(args.max), str(args.max), str(limitPerJob), str(seed), str(i*limitPerJob), "pct"])
                elif args.mode=="race":
                    os.environ["mode"]="race"
                    call_check(cmd, qsubargs + [str(0), str(args.max), str(limit), str(seed), "0", "race"])
                elif args.mode == "pb":
                    if args.job == -1 or args.job == 0:
                        call_check(cmd, qsubargs + [str(0), str(args.max), str(limit), str(seed), "0","pb"])
                elif args.mode == "db":
                    if args.job == -1 or args.job == 0:
                        call_check(cmd, qsubargs + [str(0), str(args.max), str(limit), str(seed), "0","db"])
                else:
                    assert(0)
            counter += 1

    
