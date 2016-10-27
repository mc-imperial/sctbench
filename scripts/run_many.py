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

# tests list mode outdir raceTO raceTimes dbTL pbTL 

def commandLine ():
    from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
    
    cmdline = ArgumentParser(formatter_class=ArgumentDefaultsHelpFormatter)
    
    # The command-line parser and its options
#     cmdline.add_argument("-",
#                           "--dir",
#                           action="store",
#                           help="run all tests in the directory",
#                           metavar="<DIR>")
    
#     cmdline.add_argument("-f",
#                           "--file",
#                           action="store",
#                           help="run this specific tests in the given directory",
#                           metavar="<FILE>")
    
    cmdline.add_argument(action="store",
                         help="A list of tests to run.",
                         dest="testslist")
    
    cmdline.add_argument("-wt",
                         "--walltime",
                          action="store",
                          type=str,
                          help="max wall time for each test",
                          default="09:00:00")
    
    cmdline.add_argument("-mem",
                         "--mem",
                          action="store",
                          type=str,
                          help="max RAM needed",
                          default="800mb")
    
    cmdline.add_argument("-ncpus",
                         "--ncpus",
                          action="store",
                          type=str,
                          help="#CPUs needed",
                          default="2")
    
    cmdline.add_argument("-mode",
                         "--mode",
                          action="store",
                          type=str,
                          help="race or chess",
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
    
    cmdline.add_argument("-pb",
                         "--pb",
                          action="store",
                          type=int,
                          help="",
                          default=1)
    cmdline.add_argument("-db",
                         "--db",
                          action="store",
                          type=int,
                          help="",
                          default=1)
    
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
                          help="",
                          default=10)
    
    return cmdline.parse_args()

def call_check(cmd, qsubargs):
    print cmd
    qsubargs = " ".join(qsubargs)
    os.environ["qsubargs"]=qsubargs
    subprocess.check_call(cmd)

if __name__ == "__main__":
    assert "PIN_HOME" in os.environ
    args = commandLine()
    
    script = "run_single.sh"
   # script = "run_single_race.sh" if mode == "race" else "run_single.sh"
    script = path.join(os.environ["ROOT"], "scripts", script)
    
    envfile = path.join(os.environ["ROOT"], "cluster_env.sh")
    
    outdir = path.join(os.environ["ROOT"],"benchmarks","__results",time.strftime("%Y-%m-%d-%H-%M-%S"))
    mkdir_p(outdir)

    limit = int(args.limit)
    seed = 0
    numJobs = int(args.numjobs)

    limitPerJob = limit / numJobs
    
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
            cmd = ["qsub",
                   "-l", "walltime="+args.walltime,
                   "-l", "mem="+args.mem,
                   "-l", "ncpus="+args.ncpus,
                   "-j", "oe",
                   "-N", (suite[0]+test)[0:15],
                   "-V",
                    #"--",
                    script]
            # envfile, 
            qsubargs = [suite, test, outdir,
                    str(args.timelimit), str(args.numthreads),
                    maxThreads, maxSteps]
            os.environ["mode"]=""

            if args.mode=="dfs":
                call_check(cmd, qsubargs + [str(0), str(0), str(limit), str(seed), "0", "dfs"])
            elif args.mode=="random":
                for i in range(0, numJobs):
                  call_check(cmd, qsubargs + [str(0), str(0), str(limitPerJob), str(seed), str(i*limitPerJob), "random"])
            elif args.mode=="pct":
                for i in range(0,numJobs):
                    call_check(cmd, qsubargs + [str(args.max), str(args.max), str(limitPerJob), str(seed), str(i*limitPerJob), "pct"])
            elif args.mode=="race":
                os.environ["mode"]="race"
                call_check(cmd, qsubargs + [str(0), str(args.max), str(limit), str(seed), "0", "race"])
            elif args.mode == "chess":
                #db
                if args.db > 0:
                    for i in range(0,args.max):
                        call_check(cmd, qsubargs + [str(i),str(i+1), str(limit), str(seed), "0","db"])
                    call_check(cmd, qsubargs + [str(args.max),str(100), str(limit), str(seed), "0","db"])
                #pb
                if args.pb > 0:
                    for i in range(0,args.max):
                        call_check(cmd, qsubargs + [str(i),str(i+1), str(limit), str(seed), "0","pb"])
                    call_check(cmd, qsubargs + [str(args.max),str(100), str(limit), str(seed), "0","pb"])
            else:
              assert(0)

    
