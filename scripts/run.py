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

class Command(object):
    def __init__(self, cmd):
        self.cmd = cmd
        self.process = None

    def run(self, timeout, fileout=None):
        def target():
            if fileout != None:
                with open(fileout, 'w') as f:
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
            if thread.is_alive():
                print ' Interrupted'
                #self.process.send_signal(signal.SIGINT)
                os.killpg(self.process.pid, signal.SIGKILL)
                thread.join()
                
                if fileout != None:
                    with open(fileout, 'a') as f:
                        f.write("PROBLEM: Interrupted")
                raise
        if thread.is_alive():
            print ' Timeout'
            #self.process.send_signal(signal.SIGINT)
            os.killpg(self.process.pid, signal.SIGKILL)
            #self.process.terminate()
            thread.join()
            if fileout != None:
                with open(fileout, 'a') as f:
                    f.write("TIMEOUT OCCURRED")
                    
            raise TestTimeoutException()
        return self.process.returncode

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

def setAndMkOutdir():
    outdir = path.join(os.environ["ROOT"],"benchmarks","__results",time.strftime("%Y-%m-%d-%H-%M-%S"))
    mkdir_p(outdir)
    os.environ["out_dir"]=outdir

def outfilename(boundType, boundValue, test):
    # In some cases test[0] is a not a single directory but a partial path. Therefore we have to change the                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
    dirName = test[0].replace(os.sep, '-')
    return path.join(os.environ["out_dir"],time.strftime("%Y-%m-%d-%H-%M-%S")+"--"+dirName+"--"+test[1]+"--"+boundType+"--"+boundValue+".txt")

#outfile = path.join(os.environ["out_dir"],os.environ["outname"]+"-"+time.strftime("%Y-%m-%d--%H-%M-%S")+"-{SUITE}-{TEST}.txt")

def run(test, timeout, boundType, boundValue):
    os.environ["timeout"]=str(timeout)
    cmd = [os.path.abspath(os.curdir) + os.sep + "run.sh", test[1]]
    command = Command(cmd)
    print " " + boundType + ": " + str(cmd)
    sys.stdout.flush()
    command.run(timeout=timeout, fileout=outfilename(boundType, boundValue, test))
    sys.stdout.flush()
    
def clean():
    import shutil
    resultsDir = os.path.abspath(os.curdir + os.sep + "__results")
    shutil.rmtree(resultsDir, True)
        
def commandLine ():
    from argparse import ArgumentParser, ArgumentDefaultsHelpFormatter
    
    cmdline = ArgumentParser(formatter_class=ArgumentDefaultsHelpFormatter)
    
    # The command-line parser and its options    
    cmdline.add_argument("files",
                         nargs='+',
                         help="run these specific tests")
    
    cmdline.add_argument("-t",
                          "--timeout",
                          action="store",
                          type=int,
                          help="set timeout",
                          metavar="<INT>",
                          default=600)
    
    cmdline.add_argument("-l",
                          "--limit",
                          action="store",
                          type=int,
                          help="set limit of executions",
                          metavar="<INT>",
                          default=10)
    
    cmdline.add_argument("-b",
                          action="store",
                          type=int,
                          dest="bound", 
                          help="set delay and pre-emption bound",
                          metavar="<INT>",
                          default=5)
                          
    cmdline.add_argument("--skip-preemption-bounding",
                         action="store_true",
                         help="skip preemption bounding")
    
    cmdline.add_argument("-C",
                         "--clean",
                         action="store_true",
                         help="clean out previous runs")
                          
    return cmdline.parse_args()

class TestTimeoutException(Exception):
    pass
        
if __name__ == "__main__":  
    args = commandLine()
    
    if args.clean:
        clean()
        
    for arg in args.files:
        assert os.path.exists(os.path.abspath(arg)), "The file '%s' passed on the command line does not exist" % arg
        assert os.path.splitext(arg)[1] == '.c', "The file '%s' does not have a .c extension" % arg
            
    os.environ["ROOT"] = os.path.abspath(os.curdir)
    os.environ["check_mem"]="0"
    os.environ["enable_djit"]="0"
    os.environ["sched_race"]="1"
    os.environ["sched_app"]="1"
    os.environ["unit_size"]="1"
    os.environ["seal_after_one"]="0"
    os.environ["add_races"]="0"
    os.environ["num_threads"]="4"
    os.environ["pb"]="1"
    os.environ["delay_bound"]="1"
    os.environ["bound"]="1"
    os.environ["mode"]="chess"
    os.environ["limit"]=str(args.limit)
    os.environ["output"]=""
    os.environ["run_good"]="0"
    os.environ["run_bad"]="1"
    
    setAndMkOutdir()
    
    startDir = os.path.abspath(os.curdir)
    testFile = os.path.abspath('tests.txt')
    assert os.path.exists(testFile), "File '%s' which includes test cases does not exist" % filename
    with open(testFile, 'r') as f: 
        for line in f:
            line = line.strip()                
            # Ignore commented line
            if line and not line.startswith('#'):
                test = re.split(r'\s+', line)
                assert len(test) == 2, "Line '%s' is not a valid test case" % line
                testDir = test[0]
                if os.path.isdir(os.path.abspath(testDir)):
                    testFile = testDir + os.sep + test[1] + ".c"
                    if not args.files or testFile in args.files:
                        print "Test '%s'" % testFile
                        os.chdir(testDir)   
                        print os.path.abspath(os.curdir)
                        
                        timeout    = args.timeout
                        upperRange = args.bound
                        
                        os.environ["mode"]="race" 
                        try:
                            run(test, timeout, "0race", "0")
                        except TestTimeoutException as ex:
                            print "  Timeout. Skipping race."
         
                        os.environ["mode"]="chess"
                        if args.skip_preemption_bounding:
                            #preemption bounding
                            try:
                                os.environ["pb"]="1"
                                os.environ["delay_bound"]="0"
                                for i in range(0,upperRange):
                                    os.environ["bound"]=str(i)
                                    run(test, timeout, "pb", str(i))
                            except TestTimeoutException as ex:
                                print "  Timeout. Skipping pb."
                        
                        #delay bounding
                        try:
                            os.environ["pb"]="1"
                            os.environ["delay_bound"]="1"
                            for i in range(0,upperRange):
                                os.environ["bound"]=str(i)
                                run(test, timeout, "db", str(i))
                        except TestTimeoutException as ex:
                            print "  Timeout. Skipping db."
                        os.chdir(startDir)
    
