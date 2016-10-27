#! /usr/bin/env python

import os
import sys
import subprocess
from os import path
import errno
import shutil

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

if __name__ == "__main__":
    files = [f for f in os.listdir(".") if f.endswith(".c") and os.path.isfile(f)]
    files.sort()
    failedCompilations = []
    for file in files:
        basename = file[:-2]
        dir = path.join("obj",basename, "run")
        ofile = path.join(dir, basename + ".x")
        mkdir_p(dir)
        if file=="bzip2smp.comb.c":
          shutil.copy2("bzip_input", dir)
        cmd = ["gcc", "-o", ofile, file, "-pthread", "-g"]
        print "======> BUILDING '%s'" % ' '.join(cmd)
        try:
            subprocess.check_call(cmd, stdout=None, stderr=None)
        except subprocess.CalledProcessError:
            failedCompilations.append(file)
        
    for file in failedCompilations:
        print "======> COMPILATION FAILURE: %s" % file


