##
## INTEL CONFIDENTIAL Copyright (February 2011) Intel Corporation All
## Rights Reserved.
## 
## The source code contained or described herein and all documents
## related to the source code * ("Material") are owned by Intel
## Corporation or its suppliers or licensors. Title to the Material
## remains with Intel Corporation or its suppliers and licensors. The
## Material contains trade secrets and proprietary and confidential
## information of Intel or its suppliers and licensors. The Material
## is protected by worldwide copyright and trade secret laws and
## treaty provisions. No part of the Material may be used, copied,
## reproduced, modified, published, uploaded, posted, transmitted,
## distributed, or disclosed in any way without Intel’s prior
## express written permission.
## 
## No license under any patent, copyright, trade secret or other
## intellectual property right is granted to or conferred upon you by
## disclosure or delivery of the Materials, either expressly, by
## implication, inducement, estoppel or otherwise. Any license under
## such intellectual property rights must be express and approved by
## Intel in writing.
##

import sys,os

total_seconds = 0
items_processed = 0

def extractTime(time_string):
    timelist = time_string.split()
    assert len(timelist) == 2
    timing = timelist[1]
    sec_dot = timing.split(".")
    assert len(sec_dot) == 2
    total_time = 0
    seconds = sec_dot[0]
    seconds = seconds.strip()
    seconds = seconds.strip("s")
    total_time += float(seconds)
    partial_sec = sec_dot[1]
    partial_sec = partial_sec.strip()
    partial_sec = partial_sec.strip("s")
    partial_sec = "." + partial_sec
    total_time += float(partial_sec)
    print "\t\tString:", time_string.strip()
    print "\t\tTime  :", total_time, "sec"
    return total_time

def processRun(logfile):
    assert os.path.isfile(logfile)
    print "\t", logfile
    run_log = open(logfile, "r").readlines()
    i = 0
    while i < len(run_log):
        if "real" in run_log[i]:
            return extractTime(run_log[i])
        i += 1
    else:
        return 0
        assert False, "Never found a time in this file"
                    
def sumDirectory(directory):
    total_seconds = 0
    items_processed = 0
    print "Processing:"
    for item in os.listdir(directory):
        if "run" in item and "log" not in item:
            tot_secs = processRun(os.path.join(directory, item))
            total_seconds += tot_secs
            items_processed += 1
    
    print
    print "Total time:", total_seconds
    print "Items processed:", items_processed
    if items_processed > 0:
        avg_time = round(total_seconds/items_processed,4)
        print "Average time:", avg_time
        return avg_time
    else:
        return 0

def executeDirectories(directory, run_type, out_name):
    out_log = open(out_name, "w")
    averages = {}
    for item in os.listdir(directory):
        tmp = os.path.join(directory, item, run_type)
        assert os.path.exists(tmp)
        if os.path.isdir(tmp):
            sys.stdout = out_log
            sumDirectory(tmp)
            sys.stdout = sys.__stdout__
            avg_time = sumDirectory(tmp)
            if avg_time > 0:
                averages[tmp] = avg_time

    time_info = averages.items()
    time_info.sort()
    out_log.write("\n\nFinal Averages:\n")
    for name, secs in time_info:
        out_log.write("\t" + str(name) + " : " + str(secs) + "\n")
    
def checkArgs():
    if len(sys.argv) < 4:
        print "Usage:", sys.argv[0], "[path to experiment dir] [run type] [out log]"
        print
        print "sums the times reported in the runs found in the dir,", 
        print "recording in out log."
        print "run type is used to find the run directories, it can be:"
        print "  native"
        print "  pin"
        print "  shmem"
        print "  pinplay"
        sys.exit(0)
    params = {}
    params["dir"] = sys.argv[1]
    assert os.path.exists(params["dir"])
    params["runtype"] = sys.argv[2]
    params["out"] = sys.argv[3]
    return params

def main():
    params = checkArgs()
    executeDirectories(params["dir"], params["runtype"], params["out"])

if __name__=="__main__":
    main()
