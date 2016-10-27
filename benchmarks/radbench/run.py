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

bugToExecutable = {}
bugToExecutable['bug1'] = 'test-js'
bugToExecutable['bug2'] = 'test-ctxt'
bugToExecutable['bug3'] = 'test-js'
bugToExecutable['bug4'] = 'test-time'
bugToExecutable['bug5'] = 'test-cvar'
bugToExecutable['bug6'] = 'test-rw'

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

if __name__ == "__main__":
    assert len(sys.argv) == 2, "For the radbench benchmarks you have to pass the bug number as an argument, e.g. 'bug3'"
    bugNum = sys.argv[1]
    assert bugNum in bugToExecutable, "Unable to find relevant executable for '%s'" % bugNum
    benchmarkDir = path.join(path.abspath(path.curdir), 'Benchmarks') 
    bugNumDir    = path.join(benchmarkDir, bugNum)
    runDir       = path.join(bugNumDir, 'run')
    assert path.exists(runDir), "Directory '%s' does not exist" % runDir
    os.chdir(runDir)
    fullPath = path.join(os.curdir,bugToExecutable[bugNum])
    assert path.exists(fullPath)
    assert path.isfile(fullPath)
    os.environ["output"]=""
    cmd = ["runner", fullPath]
    print cmd
    subprocess.call(cmd, stdout=None, stderr=None)

