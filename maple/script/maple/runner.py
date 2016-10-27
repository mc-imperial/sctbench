

import os, errno
import subprocess
import sys
import time
import shutil
import random
from maple.core import logging
from maple.core import static_info
from maple.core import testing
from maple.race import testing as race_testing
from maple.systematic import program
from maple.systematic import search

from maple.proto.systematic.search_pb2 import SearchInfoProto
from shutil import copyfile
import shutil
import threading
import signal

PIN_HOME = os.environ["PIN_HOME"]
MODE = "mode"
DEBUG = "debug" in os.environ
RACE = "race" in os.environ
RANDOM = "random" in os.environ
BUILD_DIR = "build-"+("debug" if DEBUG else "release")
ATTACH = "attach" in os.environ


pinbin = PIN_HOME+"/"+"pin.sh"
pintool_base = os.environ["MAPLE_HOME"]+"/"+BUILD_DIR+"/"

search_db = "search.db"
sinfo_db = "sinfo.db"
program_db = "program.db"
por_info = "por_info"
race_db = "race.db"

unit_size="1"

def done():
#    sinfo = static_info.StaticInfo()
#    sinfo.load(sinfo_db)
#    prog = program.Program(sinfo)
#    prog.load(program_db)
#    search_info = search.SearchInfo(sinfo, program)
#    search_info.load(sinfo_db)
    
    searchInfo = SearchInfoProto()
    f = open(search_db, 'rb')
    searchInfo.ParseFromString(f.read())
    f.close()
    #print len(searchInfo.node)
    #subprocess.call(["ls"])
    #print "Number of scheduling points: "+str(len(searchInfo.node))
    count=0
    avgEnabled=0
    maxEnabled=1
    numImportantSchedPoints = 0
    for n in searchInfo.node:
#        if len(n.backtrack)-len(n.done) > 1:
#            print "Error!!!!"
#            print "["
#            for m in n.enabled:
#                print " " + str(m.op)
#            print "]" 
#            print n.done
#            print n.backtrack
#            sys.exit(1)
        avgEnabled += len(n.enabled)
        maxEnabled = max(len(n.enabled), maxEnabled)
        if len(n.backtrack)-len(n.done) > 0:
            numImportantSchedPoints+=1
    #        print " selected " + str(n.sel)
    #        print " backtrack " + str(n.backtrack)
    #        print " done " + str(n.done)
    #        print count
        count+=1
    if count > 0:
        avgEnabled = avgEnabled / count
    print "Number of important scheduling points "+str(numImportantSchedPoints)
    print "Average num enabled threads "+str(avgEnabled)
    print "Max num enabled threads "+str(maxEnabled)
    sys.stdout.flush()
    return searchInfo.done
#    return search_info.done()

origdir = os.path.abspath(os.curdir)
lock = threading.RLock()
proc = None
stop = False

def afterTimeout():
    global stop
    global proc
    global lock
    with lock:
        stop = True
        if proc != None:
            try:
                os.killpg(proc.pid, signal.SIGINT)
                time.sleep(2)
                os.killpg(proc.pid, signal.SIGKILL)
            except:
                pass

timer = threading.Timer(int(os.environ["timeout"]), afterTimeout)


def main(argv):
    global pintool_base
    global pinbin
    global origdir
    global proc
    global stop
    
    try:
        os.mkdir("../run_race")
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir("../run_race"):
            pass
        else: raise
    
    pintool = ""
    if os.environ[MODE] == "chess" or os.environ[MODE] == "random" or os.environ[MODE] == "pct":
        pintool="systematic_controller.so"
    elif os.environ[MODE]=="race":
        pintool="race_pct_profiler.so"
    else:
        assert False
    
    #no longer support this
    assert not argv[0].endswith(".sh")
    if os.environ["cluster"] == "1":
        subprocess.check_call("pwd")
        
        tmpdir = os.environ["TMPDIR"]
        assert os.path.exists(argv[0])
        # copy run dir
        shutil.rmtree(os.path.join(tmpdir, "run"), True)
        shutil.copytree("../run", os.path.join(tmpdir, "run"))
        #copy run_race dir
        shutil.rmtree(os.path.join(tmpdir, "run_race"), True)
        shutil.copytree("../run_race", os.path.join(tmpdir, "run_race"))
        
        if os.environ[MODE] != "race":
            assert os.listdir(os.path.join(tmpdir, "run_race")).count("race.db") > 0
        
        #copy binary - replaces if already exists
        shutil.copy2(argv[0], os.path.join(tmpdir, "run"))
        #copy the pin tool - replaces if already exists
        shutil.copy2(pintool_base+pintool, os.path.join(tmpdir, "run"))
        #copy pin
        shutil.rmtree(os.path.join(tmpdir, "pin"), True)
        shutil.copytree(PIN_HOME, os.path.join(tmpdir, "pin"))
        
        pintool_base = ""
        pinbin = os.path.join(tmpdir, "pin", "pin.sh")
        os.chdir(tmpdir)
        subprocess.check_call(["find", ".", "-maxdepth", "2"])
        os.chdir(os.path.join(tmpdir,"run"))
        
        
    # if len(argv) < 1:
    #     logging.err(command_usage())
    # command = argv[0]
    # logging.msg('performing command: %s ...\n' % command, 2) 
    # if valid_command(command):
    #     eval('__command_%s(argv[1:])' % command)
    # else:
    #     logging.err(command_usage())
    
    if not "limit" in os.environ:
        os.environ["limit"] = "3"
    
    if not "bound" in os.environ:
        print "Need to set bound"
        sys.exit(2)
    bound = int(os.environ["bound"])
    
#    if not "pb" in os.environ:
#        print "Need to set pb"
#        sys.exit(2)
#    pb = int(os.environ["pb"])
    #if not "por" in os.environ:
    #    print "Need to set por"
    #    sys.exit(2)
    #por = int(os.environ["por"])
    #assert por == 0 or por == 1
    por = 0
    outContents = None
    
    print "Starting run"
    sys.stdout.flush()
    
    outFilename = os.environ["output"]
    if len(outFilename) > 0:
        try:
            os.remove(outFilename)
        except OSError:
            pass
    
    try:
        os.remove("program.db")
    except OSError:
        pass
    try:
        os.remove("race.db")
    except OSError:
        pass
    try:
        os.remove("search.db")
    except OSError:
        pass
    try:
        os.remove("sinfo.db")
    except OSError:
        pass
    
    os.environ["LD_LIBRARY_PATH"]=os.curdir+os.pathsep+os.environ.get("LD_LIBRARY_PATH", "")
    
    if os.environ[MODE] == "pct" or os.environ[MODE] == "random":
        runSeed = int(os.environ["seed"])
        startIndex = int(os.environ["start_index"])
        random.seed(runSeed)
        # fast forward
        for i in range(0,startIndex):
            random.getrandbits(32)
        
    runStart = time.time()
    
    for i in range(1,int(os.environ["limit"])+1):
        
        executionSeed = 0

        if not MODE in os.environ:
            print "Need to set mode env var"
            sys.exit(2)
        
        if os.environ[MODE] == "pct" or os.environ[MODE] == "random":
            executionSeed = random.getrandbits(32)
            # This converts executionSeed to a 32 bit signed int
            executionSeed -= (executionSeed & 0x80000000) << 1

        print "Starting execution"
        sys.stdout.flush()
        executionStart=time.time()
        
        pincmd=""
        if os.environ[MODE]=="race":
            pincmd = [pinbin,
            "-t", 
            pintool_base+"race_pct_profiler.so", 
            "-race_in", 
            "../run_race/"+race_db, 
            "-race_out", 
            "../run_race/"+race_db,
            "-pct_history", 
            "../run_race/pct.histo", 
            "-sinfo_in", 
            "../run_race/"+sinfo_db,  
            "-sinfo_out", 
            "../run_race/"+sinfo_db, 
            "-stat_out", 
            "../run_race/stat.out", 
            "-ignore_lib", 
            "0", 
            "-enable_djit", 
            "1",
            "-enable_debug", 
            "1" if ATTACH else "0", 
            "-count_mem", 
            "1", 
            "-debug_syscall", 
            "0", 
            "-debug_call_return", 
            "0", 
            "-strict", 
            "0", 
            "-debug_track_clk", 
            "1", 
            "-unit_size", 
            unit_size, 
            "-track_racy_inst", 
            "0", 
            "-debug_pthread", 
            "0", 
            "-debug_track_callstack", 
            "0", 
            "-debug_atomic", 
            "0", 
            "-debug_malloc", 
            "0", 
            "-debug_mem", 
            "0", 
            "-debug_out", 
            "stdout", 
            "-debug_main", 
            "0", 
            "-depth", 
            "3", 
            "-cpu", 
            os.environ["cpu"], 
            "--"]
        elif os.environ[MODE]=="random":
            pincmd = [pinbin,
            "-t",pintool_base+"systematic_controller.so",
            "-enable_debug", "1" if ATTACH else "0",
            "-search_in", search_db,
            "-search_out",search_db,
            "-race_in", "../run_race/"+race_db,
            "-race_out",(("../run_race/"+race_db) if os.environ["add_races"]=="1" else race_db),
            "-sinfo_in","../run_race/"+sinfo_db,
            "-sinfo_out", (("../run_race/"+sinfo_db) if os.environ["add_races"]=="1" else sinfo_db),
            "-program_in", program_db,
            "-program_out",program_db,
            "-enable_random_scheduler", "1",
            "-enable_chess_scheduler", "0",
            "-unit_size", unit_size,
            "-check_mem", os.environ["check_mem"],
            "-enable_djit", os.environ["enable_djit"],
            "-sched_race", os.environ["sched_race"],
            "-sched_app", os.environ["sched_app"],
            "-seal_after_one", os.environ["seal_after_one"],
            "-por", str(por),
            "-fair", "0",
            "-pb", os.environ["pb"],
            "-delay_bound", os.environ["delay_bound"],
            "-pb_limit", str(bound),
            "-por_info_path", por_info,
            "-debug_syscall", "0",
            "-debug_call_return", "0",
            "-debug_track_clk", "1",
            "-abort_diverge", "1",
            "-track_racy_inst", "0",
            "-debug_pthread", "0",
            "-debug_track_callstack","0",
            "-realtime_priority", "1",
            "-debug_atomic", "0",
            "-debug_malloc", "0",
            "-stat_out", "stat.out",
            "-debug_out", "stdout",
            "-debug_main", "0",
            "-cpu", os.environ["cpu"],
            "-debug_mem", "0",
            "-seed", str(executionSeed),
            "-use_seed", "1",
            "--"
            ]
        elif os.environ[MODE]=="pbzero":
            pincmd = [pinbin,
            "-t",pintool_base+"systematic_controller.so",
            "-enable_random_scheduler", "0",
            "-enable_chess_scheduler", "1",
            "-enable_debug", "1" if ATTACH else "0",
            "-race_in", "../"+race_db,
            "-race_out","../"+race_db,
            "-sinfo_in","../"+sinfo_db,
            "-sinfo_out", "../"+sinfo_db,
            "-search_in", search_db,
            "-search_out",search_db,
            "-program_in", program_db,
            "-program_out",program_db,
            "-por_info_path", por_info,
            "-sched_race", "0",
            "-enable_djit", "1",
            "-unit_size", unit_size,
            "-por", "0",
            "-fair", "1",
            "-pb", "1",
            "-pb_limit", "0",
            "-sched_app", "1",
            "-debug_syscall", "0",
            "-debug_call_return", "0",
            "-debug_track_clk", "1",
            "-abort_diverge", "1",
            "-track_racy_inst", "0",
            "-debug_pthread", "0",
            "-debug_track_callstack","0",
            "-realtime_priority", "1",
            "-debug_atomic", "0",
            "-debug_malloc", "0",
            "-stat_out", "stat.out",
            "-debug_out", "stdout",
            "-debug_main", "0",
            "-cpu", os.environ["cpu"],
            "-debug_mem", "0",
            "--"
            ]
        elif os.environ[MODE]=="chess":
            pincmd = [pinbin,
            "-t",pintool_base+"systematic_controller.so",
            "-enable_debug", "1" if ATTACH else "0",
            "-search_in", search_db,
            "-search_out",search_db,
            "-race_in", "../run_race/"+race_db,
            "-race_out",(("../run_race/"+race_db) if os.environ["add_races"]=="1" else race_db),
            "-sinfo_in","../run_race/"+sinfo_db,
            "-sinfo_out", (("../run_race/"+sinfo_db) if os.environ["add_races"]=="1" else sinfo_db),
            "-program_in", program_db,
            "-program_out",program_db,
            "-enable_random_scheduler", "0",
            "-enable_chess_scheduler", "1",
            "-unit_size", unit_size,
            "-check_mem", os.environ["check_mem"],
            "-enable_djit", os.environ["enable_djit"],
            "-sched_race", os.environ["sched_race"],
            "-sched_app", os.environ["sched_app"],
            "-seal_after_one", os.environ["seal_after_one"],
            "-por", str(por),
            "-fair", "0",
            "-pb", os.environ["pb"],
            "-delay_bound", os.environ["delay_bound"],
            "-pb_limit", str(bound),
            "-por_info_path", por_info,
            "-debug_syscall", "0",
            "-debug_call_return", "0",
            "-debug_track_clk", "1",
            "-abort_diverge", "1",
            "-track_racy_inst", "0",
            "-debug_pthread", "0",
            "-debug_track_callstack","0",
            "-realtime_priority", "1",
            "-debug_atomic", "0",
            "-debug_malloc", "0",
            "-stat_out", "stat.out",
            "-debug_out", "stdout",
            "-debug_main", "0",
            "-cpu", os.environ["cpu"],
            "-debug_mem", "0",
            "--"
            ]
        elif os.environ[MODE]=="pct":
            pincmd = [pinbin,
            "-t",pintool_base+"systematic_controller.so",
            "-enable_debug", "1" if ATTACH else "0",
            "-search_in", search_db,
            "-search_out",search_db,
            "-race_in", "../run_race/"+race_db,
            "-race_out",(("../run_race/"+race_db) if os.environ["add_races"]=="1" else race_db),
            "-sinfo_in","../run_race/"+sinfo_db,
            "-sinfo_out", (("../run_race/"+sinfo_db) if os.environ["add_races"]=="1" else sinfo_db),
            "-program_in", program_db,
            "-program_out",program_db,
            "-enable_random_scheduler", "0",
            "-enable_chess_scheduler", "0",
            "-enable_pct_scheduler", "1",
            "-pct_n", os.environ["max_threads"],
            "-pct_k", os.environ["max_steps"],
            "-pct_d", os.environ["bug_depth"],
            "-unit_size", unit_size,
            "-check_mem", os.environ["check_mem"],
            "-enable_djit", os.environ["enable_djit"],
            "-sched_race", os.environ["sched_race"],
            "-sched_app", os.environ["sched_app"],
            "-seal_after_one", os.environ["seal_after_one"],
            "-por", str(por),
            "-fair", "0",
            "-pb", os.environ["pb"],
            "-delay_bound", os.environ["delay_bound"],
            "-pb_limit", str(bound),
            "-por_info_path", por_info,
            "-debug_syscall", "0",
            "-debug_call_return", "0",
            "-debug_track_clk", "1",
            "-abort_diverge", "1",
            "-track_racy_inst", "0",
            "-debug_pthread", "0",
            "-debug_track_callstack","0",
            "-realtime_priority", "1",
            "-debug_atomic", "0",
            "-debug_malloc", "0",
            "-stat_out", "stat.out",
            "-debug_out", "stdout",
            "-debug_main", "0",
            "-cpu", os.environ["cpu"],
            "-debug_mem", "0",
            "-seed", str(executionSeed),
            "-use_seed", "1",
            "--"
            ]
        else:
            print "Need to set mode env var"
            sys.exit(2)
        pincmd.insert(1, "child")
        pincmd.insert(1, "-injection")
        if ATTACH:
            pincmd.insert(1, "10")
            pincmd.insert(1, "-pause_tool")
#            pincmd.insert(1, "0")
#            pincmd.insert(1, "-inline")
#            pincmd.insert(1, "-appdebug")
        print " ".join(pincmd+argv)
        
        outContentsAfter = None
        
        if len(outFilename) > 0 and outContents == None:
            try:
                with open(outFilename, 'rb') as outFile:
                    outContents = outFile.read()
            except IOError as e:
                print "WARNING: output file "+outFilename+" NOT read."
        
#         if argv[0].endswith(".sh"):
#             print "Detected script."
#             os.environ["PINCMD"] = " ".join(pincmd)
#             sys.stdout.flush()
#             subprocess.call(argv)
#         else:
        sys.stdout.flush()
        #subprocess.call(pincmd + argv)
        with lock:
            if stop:
                break
            proc = subprocess.Popen(pincmd + argv, preexec_fn=os.setsid)
        proc.communicate()
        with lock:
            if stop:
                break
        
        if len(outFilename) > 0 and outContents != None:
            try:
                with open(outFilename, 'rb') as outFile:
                    outContentsAfter = outFile.read()
                if outContents != outContentsAfter:
                    print "ERROR: Output "+outFilename+" did not match first output!"
                    copyfile(outFilename, "DIFFERENT_OUT.txt")
            except IOError as e:
                print "ERROR: Failed to read output file " + outFilename + "!"
        
        
        executionEnd=time.time()
        
        print "finished execution " + str(i)
        print "execution time: " + str(executionEnd-executionStart) + " seconds"
        sys.stdout.flush()
        sys.stderr.flush()
        
        
        if os.environ[MODE] != "random" and os.environ[MODE] != "pct":
            if os.environ[MODE] != "chess" or proc.returncode == 77:
                print "NO MORE EXECUTIONS"
                break
    
    runEnd = time.time()
    
    print "run time: " + str(runEnd-runStart) + " seconds"
    
    if os.environ["cluster"] == "1" and os.environ[MODE]=="race":
        print "Copying run_race back to home."
        sys.stdout.flush()
        #shutil.copy2("../run_race/race.db", os.path.join(origdir,"..","run_race","race.db"))
        
        shutil.rmtree(os.path.join(origdir,"..","run_race"), True)
        shutil.copytree("../run_race", os.path.join(origdir,"..","run_race"))
    
    #proc = subprocess.Popen(args=" ".join(ex))
    if stop:
        print "TIMEOUT OCCURRED"
    sys.stdout.flush()
    

if __name__ == '__main__':
    try:
        if os.environ["mode"]=="race":
            timer.start()
        main(sys.argv[1:])
    finally:
        if os.environ["mode"]=="race":
            timer.cancel()


