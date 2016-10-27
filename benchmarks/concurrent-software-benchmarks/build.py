#! /usr/bin/env python

import os
import sys
import subprocess
from os import path
import errno

files = [f for f in os.listdir(".") if f.endswith(".c") and os.path.isfile(f)]

files.sort()

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

for file in files:
    basename = file[:-2]
    dir = path.join("obj",basename, "run")
    ofile = path.join(dir, basename + ".x")
    mkdir_p(dir)
    cmd = ["gcc", "-o", ofile, file, "-pthread", "-g", "-O0"]
    print " ".join(cmd)
    subprocess.check_call(cmd)



