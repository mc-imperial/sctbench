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

class TestTimeoutException(Exception):
    pass

class Command(object):
    def __init__(self, cmd):
        self.cmd = cmd
        self.process = None

    def run(self, timeout, fileout=None):
        def target():
            if fileout != None:
                with open(fileout, 'w') as f:
                    print self.cmd
                    self.process = subprocess.Popen(self.cmd, stdout=f, stderr=subprocess.STDOUT, preexec_fn=os.setsid)
                    self.process.communicate()
            else:
                self.process = subprocess.Popen(self.cmd, preexec_fn=os.setsid)
                self.process.communicate()
            
        
        thread = threading.Thread(target=target)
        timeoutOccurred = False
        try:
            thread.start()
            thread.join(timeout)
        except:
            if True:
                print ' Interrupted'
                try:
                    os.killpg(self.process.pid, signal.SIGINT)
                    time.sleep(2)
                    os.killpg(self.process.pid, signal.SIGKILL)
                except:
                    pass
                thread.join()
                
                if fileout != None:
                    with open(fileout, 'a') as f:
                        f.write("PROBLEM: Interrupted")
                raise
        if thread.is_alive():
            print ' Timeout'
            try:
                os.killpg(self.process.pid, signal.SIGINT)
                time.sleep(2)
                os.killpg(self.process.pid, signal.SIGKILL)
            except:
                pass
            thread.join()
            if fileout != None:
                with open(fileout, 'a') as f:
                    f.write("TIMEOUT OCCURRED")
                    
            raise TestTimeoutException()
        return self.process.returncode


def getExecutionInfo(outfile):
    numExecutions = 0
    timeout = False
    with open(outfile, 'r') as f:
        for line in f:
            if line.startswith("finished execution"):
                m = re.search("finished execution (\d+)", line)
                numExecutions = int(m.group(1))
            elif line.startswith("TIMEOUT OCCURRED"):
                timeout = True
    return numExecutions, timeout

def outfilename(boundType, boundValue, suite, test, outdir):
    return path.join(outdir,time.strftime("%Y-%m-%d-%H-%M-%S")+"--"+suite+"--"+test+"--"+boundType+"--"+boundValue+"--.txt")

def run(test, timeout, outfile):
    #print os.environ["PBS_NODENUM"], os.environ["PBS_TASKNUM"], os.environ["NCPUS"]
    cmd = [path.join(".","run.sh"), test]
    sys.stdout.flush()
    os.environ["timeout"] = str(timeout)
    fulloutfile = outfile
    if os.environ["cluster"]=="1":
        outfile = path.join(os.environ["TMPDIR"], path.basename(outfile))
    command = Command(cmd)
    command.run(timeout+20, outfile)
    #with open(outfile, 'w') as f:
    #    subprocess.call(cmd, stdout=f, stderr=subprocess.STDOUT, preexec_fn=os.setsid)
    sys.stdout.flush()
    if os.environ["cluster"]=="1":
        print "copying outfile from TMPDIR"
        shutil.copy2(outfile, fulloutfile)
        sys.stdout.flush()

if __name__ == "__main__":
    assert "PIN_HOME" in os.environ
    
    cmd_name = sys.argv.pop(0)
    
    suite = sys.argv.pop(0)
    test = sys.argv.pop(0)
    outdir = sys.argv.pop(0)
    timelimit = int(sys.argv.pop(0))
    numThreads = sys.argv.pop(0)
    maxThreads = sys.argv.pop(0)
    maxSteps = sys.argv.pop(0)
    min = int(sys.argv.pop(0))
    max = int(sys.argv.pop(0))
    limit = int(sys.argv.pop(0))
    seed = int(sys.argv.pop(0))
    startIndex = int(sys.argv.pop(0))
    mode = sys.argv.pop(0)
    
    if len(sys.argv) != 0:
        print "USAGE: ", cmd_name, "suite test outdir timelimit numThreads maxThreads maxSteps min max limit seed startIndex mode"
        sys.exit(1)
    
    
    os.chdir(path.join( os.environ["ROOT"] , "benchmarks" , suite ))
    
    os.environ["check_mem"]="0"
    os.environ["enable_djit"]="0"
    os.environ["sched_race"]="1"
    os.environ["sched_app"]="1"
    os.environ["unit_size"]="1"
    os.environ["seal_after_one"]="0"
    os.environ["add_races"]="0"
    os.environ["num_threads"]=numThreads
    os.environ["pb"]="1"
    os.environ["delay_bound"]="1"
    os.environ["bound"]="1"
    os.environ["mode"]="chess"
    os.environ["limit"]=str(limit)
    os.environ["output"]=""
    
    os.environ["cpu"]="0"
    
    os.environ["max_threads"]=str(maxThreads)
    os.environ["max_steps"]=str(maxSteps)
    
    os.environ["start_index"]=str(startIndex)
    os.environ["seed"]=str(seed)
    
    if mode == "race":
        os.environ["mode"]="race" 
        for i in range(min,max):
            outfile = outfilename("0race", str(i), suite, test, outdir)
            run(test, timelimit, outfile)
    
    if mode == "dfs":
        os.environ["mode"]="chess"
        os.environ["pb"]="0"
        os.environ["delay_bound"]="0"
        outfile = outfilename("dfs", "0", suite, test, outdir)
        run(test, timelimit, outfile)

    if mode == "random":
        os.environ["mode"]="random"
        outfile = outfilename("random", "0,"+str(seed)+","+str(startIndex), suite, test, outdir)
        run(test, timelimit, outfile)
    
    if mode == "pct":
        os.environ["mode"]="pct"
        os.environ["bug_depth"]=str(min)
        outfile = outfilename("pct", str(min)+","+str(seed)+","+str(startIndex), suite, test, outdir)
        run(test, timelimit, outfile)
    
    if mode == "db":
        # delay bounding
        previousNumExecutions = 0
        os.environ["mode"]="chess"
        os.environ["pb"]="1"
        os.environ["delay_bound"]="1"
        for i in range(min,max):
            os.environ["bound"]=str(i)
            outfile = outfilename("db", str(i), suite, test, outdir)
            run(test, timelimit, outfile)
            numEx, to = getExecutionInfo(outfile)
            if to:
                print " Timeout! DB done."
                break
            if numEx == previousNumExecutions:
                print " Same # of executions. DB done."
                break
            if numEx == int(os.environ["limit"]):
                print " Hit schedule limit. DB done."
                break
            previousNumExecutions = numEx
            
    
    if mode == "pb":
        # preemption bounding
        previousNumExecutions = 0
        os.environ["mode"]="chess"
        os.environ["pb"]="1"
        os.environ["delay_bound"]="0"
        for i in range(min,max):
            os.environ["bound"]=str(i)
            outfile = outfilename("pb", str(i), suite, test, outdir)
            run(test, timelimit, outfile)
            numEx, to = getExecutionInfo(outfile)
            if to:
                print " Timeout! PB done."
                break
            if numEx == previousNumExecutions:
                print " Same # of executions. PB done."
                break
            if numEx == previousNumExecutions:
                break
            if numEx == int(os.environ["limit"]):
                print " Hit schedule limit. PB done."
                break
            previousNumExecutions = numEx
    
