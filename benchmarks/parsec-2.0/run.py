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

# class Command(object):
#     def __init__(self, cmd):
#         self.cmd = cmd
#         self.process = None

#     def run(self, timeout, fileout=None):
#         def target():
#             if fileout != None:
#                 with open(fileout, 'w') as f:
#                     self.process = subprocess.Popen(self.cmd, stdout=f, stderr=subprocess.STDOUT)
#             else:
#                 self.process = subprocess.Popen(self.cmd)
#             self.process.communicate()
            

#         thread = threading.Thread(target=target)
#         thread.start()

#         thread.join(timeout)
#         if thread.is_alive():
#             print 'Terminating process'
#             self.process.terminate()
#             thread.join()
#             if fileout != None:
#                 with open(fileout, 'a') as f:
#                     f.write("TIMEOUT OCCURRED")
#         return self.process.returncode


outputs = {
         #bad
         "streamcluster":"output.txt",
         "streamcluster2":"output.txt",
         "streamcluster3":"output.txt",
         "ferret":"output.txt",
         #good
         "raytrace": "output.bin",
         "vips": "output.v",
         "bodytrack": "sequenceB_1/poses.txt",
         "fluidanimate": "out.fluid",
         "facesim": "",
         "canneal": ""
         }

#for (test,output) in tests:
testName = sys.argv[1]
os.environ["output"]=outputs[testName]
#outfile = path.join(os.environ["out_dir"],os.environ["outname"]+"-"+time.strftime("%Y-%m-%d--%H-%M-%S")+"-parsec-"+test+".txt")
# todo: input size is currently "test"
cmd = ("./parsecrun.sh -k -a run -p "+testName+" -s runner -n "+str(os.environ["num_threads"])+" -i "+"test"+"").split(" ")
print cmd
#command = Command(cmd)
#command.run(timeout=int(os.environ["timeout"]), fileout=outfile)
subprocess.call(cmd)


