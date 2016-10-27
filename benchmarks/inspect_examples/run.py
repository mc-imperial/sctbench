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

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

if __name__ == "__main__":
    startdir = path.abspath(path.curdir)
    file=sys.argv[1]+".c"
    
    args = ""
    os.chdir(startdir)
    first_line = ""
    with open(file, 'r') as f:
        first_line = f.readline()
    if "args:" in first_line:
        os.environ["output"]="output.txt"
        m = re.search(".*args[:](.*)", first_line)
        args = m.group(1).format(NUMTHREADS=os.environ["num_threads"], OUTFILE="output.txt")
    else:
        os.environ["output"]=""
    basename = file[:-2]
    os.chdir(path.join("obj",basename, "run"))
    ex = basename + ".x"
    cmd = ["runner", path.join(os.curdir,ex)]
    cmd.extend(args.split(" "))
    print cmd
    subprocess.call(cmd, stdout=None, stderr=None)

