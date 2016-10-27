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
    needMathLibrary = ['swarm_isort64.comb.c']
    files = [f for f in os.listdir(".") if f.endswith(".c") and os.path.isfile(f)]
    files.sort()
    failedCompilations = []
    for file in files:
        basename = file[:-2]
        dir = path.join("obj",basename, "run")
        ofile = path.join(dir, basename + ".x")
        mkdir_p(dir)
        if file == "pfscan.comb.c":
            shutil.rmtree(path.join(dir, "pfscandir"), True)
            shutil.copytree("pfscandir", path.join(dir, "pfscandir"))
        cmd = ["gcc", "-o", ofile, file, "-pthread", "-g"]
        if file in needMathLibrary:
            cmd.append("-lm")
        print "======> BUILDING '%s'" % ' '.join(cmd)
        try:
            subprocess.check_call(cmd, stdout=None, stderr=None)
        except subprocess.CalledProcessError:
            failedCompilations.append(file)
    for file in failedCompilations:
        print "======> COMPILATION FAILURE: %s" % file


