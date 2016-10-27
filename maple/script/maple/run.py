

import os, errno
import subprocess
import sys
import glob
import shutil
import time

def remove_dir(dirname):
    for f in glob.iglob("pkgs/apps/*/"+dirname):
        print "removing", f
        shutil.rmtree(f)
    for f in glob.iglob("pkgs/kernels/*/"+dirname):
        print "removing", f
        shutil.rmtree(f)

def clear_race():
    remove_dir("run_race")

def clear_runs():
    remove_dir("run")

def race_mode():
    os.environ["mode"]="race"

def random_mode(limit):
    os.environ["mode"]="random"
    os.environ["limit"]=str(limit)

def chess_mode(limit):
    os.environ["mode"]="chess"
    os.environ["limit"]=str(limit)
    
def run(prog="blackscholes",nthreads=4,size="test",stdout1=sys.stdout):
    subprocess.call(("parsecmgmt -a run -p "+prog+" -s runner -n "+str(nthreads)+" -i "+size+"").split(" "),stdout=stdout1)



tests = [("ferret", "output.txt")]
#,"ferret","raytrace","swaptions","streamcluster"]
#tests = [
#         ("streamcluster","output.txt"),
#         ("streamcluster2","output.txt")
#         ("streamcluster3","output.txt"),
#         ("ferret","output.txt"),
#         ("raytrace", "output.bin"),
#         ("vips", "output.v")
#         ("bodytrack", "sequenceB_1/poses.txt")
#         ("fluidanimate", "out.fluid")
#         ("facesim", "")
#         ("canneal", "")
#         ]

#tests = [
#("splash2x.barnes", ""),
# ("splash2x.fmm", ""),
##oom
# ("splash2x.ocean_cp", ""),
##seg fault - even native
#("splash2x.ocean_ncp", ""),
##oom
#("splash2x.radiosity", ""),
## slow
#("splash2x.raytrace", ""),
## bit slow
#("splash2x.volrend", ""),
## bit slow
#("splash2x.water_nsquared", ""),
## bit slow
#("splash2x.water_spatial", ""),
## very slow
#("splash2x.cholesky", ""),
##oom
#("splash2x.fft", ""),
##slow
#("splash2x.lu_cb", ""),
## slow
#("splash2x.lu_ncb", ""),
# 10 secs
#("splash2x.radix", "")
#         ]

#tests = [("bodytrack", "sequenceB_1/poses.txt")]

#tests = [("x264","eledream.264")]

def runrace():
    os.environ["bound"]="1"
    os.environ["por"]="0"
    os.environ["output"]=""
    
    race_mode()
    for (test,out) in tests:
        for i in range(1,2):
            run(test,2,"test")

def testrun(nthreads=2):
    chess_mode(2)
    os.environ["check_mem"]="0"
    os.environ["enable_djit"]="0"
    os.environ["sched_race"]="1"
    os.environ["sched_app"]="1"
    os.environ["por"]="0"
    
    os.environ["seal_after_one"]="0"
    
    os.environ["add_races"]="0"
    
#    os.environ["debug"]="1"
#    os.environ["attach"]="1"
    
    os.environ["pb"]="1"
    os.environ["delay_bound"]="1"
    os.environ["bound"]="1"
    
    for (test,output) in tests:
        os.environ["output"]=output
        with open(time.strftime("%Y-%m-%d--%H-%M-%S")+"-"+test+".txt", 'w') as myfile:
            run(test,nthreads,"test",stdout1=myfile)
    
#    os.environ["pb"]="0"
#    os.environ["delay_bound"]="0"
#    os.environ["bound"]="0"
#    
#    for (test,output) in tests:
#        os.environ["output"]=output
#        with open(time.strftime("%Y-%m-%d--%H-%M-%S")+"-"+test+".txt", 'w') as myfile:
#            run(test,nthreads,"test",stdout1=myfile)
#    
#    os.environ["pb"]="0"
#    os.environ["delay_bound"]="0"
#    os.environ["bound"]="0"
#    
#    for (test,output) in tests:
#        os.environ["output"]=output
#        with open(time.strftime("%Y-%m-%d--%H-%M-%S")+"-"+test+".txt", 'w') as myfile:
#            run(test,nthreads,"test",stdout1=myfile)
    
#    os.environ["bound"]="1"
#    os.environ["por"]="1"
#    for test in tests:
#        with open(time.strftime("%Y-%m-%d--%H-%M-%S")+"-"+test+".txt", 'w') as myfile:
#            run(test,nthreads,"test",stdout1=myfile)
#    
#    os.environ["bound"]="2"
#    os.environ["por"]="0"
#    for test in tests:
#        with open(time.strftime("%Y-%m-%d--%H-%M-%S")+"-"+test+".txt", 'w') as myfile:
#            run(test,nthreads,"test",stdout1=myfile)
#    
#    os.environ["bound"]="2"
#    os.environ["por"]="1"
#    for test in tests:
#        with open(time.strftime("%Y-%m-%d--%H-%M-%S")+"-"+test+".txt", 'w') as myfile:
#            run(test,nthreads,"test",stdout1=myfile)
    
    
