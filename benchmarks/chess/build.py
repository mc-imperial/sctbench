#! /usr/bin/env python

import os
import sys
import subprocess
from os import path
import errno

files = [
         "InterlockedWorkStealQueue.cpp",
         "InterlockedWorkStealQueueWithState.cpp",
         "StateWorkStealQueue.cpp",
         "WorkStealQueue.cpp"
         ]

files.sort()

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

for file in files:
    basename = file[:-4]
    dir = path.join("obj",basename, "run")
    ofile = path.join(dir, basename + ".x")
    mkdir_p(dir)
    cmd = ["g++", "-o", ofile, file, "-std=c++0x", "-pthread", "-g", "-O0"]
    print " ".join(cmd)
    subprocess.check_call(cmd)



