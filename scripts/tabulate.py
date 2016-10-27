#! /usr/bin/env ipython

import os
import re
from os import path
import sys

testNames = set()
files = []

def printTuple(tuple):
    for index, item in enumerate(tuple):
        #if index > 0:
        #    sys.stdout.write(",")
        sys.stdout.write(item)
        sys.stdout.write(",")

def getResultFromFile(filepath):
    numSchedStr=""
    numErrorsStr=""
    firstErrorStr=""
    ratioStr=""
    timeoutStr=""
    
    if filepath != "":
        num_errors=0
        num_schedules=0
        first_error=-1
        exploredAll=False
        foundConfig=False
        foundErrorThisSchedule=False
        with open(filepath, 'r') as fh:
            for line in fh:
                if not foundConfig:
                    if "race_pct_profiler" in line:
                            assert False and "Trying to tabulate a race detection file "+filepath
                    if "sched_app" in line:
    #                     if "delay_bound 1" in line:
    #                         bounding="db"
    #                     else:
    #                         bounding="pb"
    #                     m = re.search("pb_limit (\d)", line)
    #                     bound = m.group(1)
                        foundConfig=True
                if "ERROR" in line and not foundErrorThisSchedule:
                    num_errors+=1
                    foundErrorThisSchedule = True
                    if first_error == -1:
                        first_error = num_schedules 
                elif "Starting execution" in line:
                    num_schedules+=1
                    foundErrorThisSchedule = False
                elif "NO MORE EXECUTIONS" in line:
                    exploredAll=True
                elif "Number of preemptions/delays:" in line:
                    int(line.split(":")[1].strip())
                #races
        ratio = 0
        if num_schedules > 0:
            ratio = int((float(num_errors) / float(num_schedules))*100)
        
        numSchedStr = str(num_schedules)
        numErrorsStr = str(num_errors)
        firstErrorStr = str(first_error)
        ratioStr = str(ratio)+"%"
        timeoutStr = "" if exploredAll else "*"
        
    return [numSchedStr,
    numErrorsStr,
    firstErrorStr,
    ratioStr,
    timeoutStr
    ]
    

def getResult(testName, boundType, boundValue):
    assert isinstance(boundValue, str)
    theFile = ""
    for f in files:
        #m = re.match(".*?--(.*?--.*?)--(.*?)--(.*?)[.]txt", f)
        parts = f.split("--")
        if (
            testName   == parts[1]+"--"+parts[2] and 
            boundType  == parts[3] and 
            boundValue == parts[4]):
            if theFile != "":
                assert False and "found more than one matching file: " + theFile + " and " + f
            theFile = f
    
    return getResultFromFile(theFile)
    
upperRange = 0
os.chdir("__results")
for aset in os.listdir("."):
    if "use" in aset:
        for f in os.listdir(aset):
            fileName = path.join(aset,f)
            files.append(fileName)
            #m = re.match(".*?--(.*?--.*?).*?[.]txt", fileName)
            parts = fileName.split("--")
            name = parts[1]+"--"+parts[2]
            testNames.add(name)
            if parts[3] == "pb" or parts[3] == "db":
                upperRange = max(int(parts[4]), upperRange)

table = []
usefulTable = []


width=5
     
num_sched=0
num_buggy=1
first_buggy=2
perc_buggy=3
completed=4

def firstBuggyBound(row, startAt, endAt):
    startAt+=1
    endAt+=1
    i=startAt
    res=0
    comp = True
    while i < endAt and comp and row[i+first_buggy]!="":
        firstBuggy=int(row[i+first_buggy])
        comp = (row[i+completed] == "")
        if firstBuggy != -1:
            return res, i
        i+=width
        res+=1
    return -1, -1


def firstBuggy(row, startAt, endAt):
    i=startAt
    comp = True
    while i < endAt and comp and row[i+first_buggy]!="":
        firstBuggy=int(row[i+first_buggy],10)
        comp = (row[i+completed] == "")
        if firstBuggy != -1:
            return int(row[i+first_buggy]), comp
        i+=width
    return notFound, comp


for testName in sorted(list(testNames)):
    
    dataList = []
    usefulData = []
    
    dataList += [testName, "|"]
    usefulData += [testName, "|"]
    
    dataList += [testName, "||"]
    
    for i in range(0,upperRange):
        dataList+=getResult(testName, "pb", str(i))
    
    dataList += ["|||"]
    
    for i in range(0,upperRange):
        res = getResult(testName, "db", str(i))
        dataList += res
        if i==1:
            usefulData+= [res[0], "||"]
    
    dataList += ["||||"]
    
    pbBoundOfFirstBug, pbOffsetOfFirstBug = firstBuggyBound(dataList, dataList.index("||"), dataList.index("|||"))
    dbBoundOfFirstBug, dbOffsetOfFirstBug = firstBuggyBound(dataList, dataList.index("|||"), dataList.index("||||"))
    
    # pb first bug
    usefulData += [pbBoundOfFirstBug ]
    if pbOffsetOfFirstBug != -1:
        usefulData += dataList[pbOffsetOfFirstBug:pbOffsetOfFirstBug+width]
    usefulData += ["|||"]
    
    #db first bug
    usefulData += [dbBoundOfFirstBug ]
    if dbOffsetOfFirstBug != -1:
        usefulData += dataList[dbOffsetOfFirstBug:dbOffsetOfFirstBug+width]
    usefulData += ["||||"]
    
    # num of schedules for db 1 (number of scheduling points)
    table += [dataList]
    usefulTable += [usefulData]

print table
print " "
print " "
print " "
print usefulTable

# for f in files:
#     name=""
#     bounding=""
#     bound="0"
#     num_errors=0
#     num_schedules=0
#     exploredAll=False
#     foundConfig=False
#     
#     if "0race" in f:
#         continue
#     
#     m = re.match(".*--(.*)--(.*)--(.*)--(.*)[.]txt", f)
#     name = m.group(1)+"-"+m.group(2)
#     boundType = m.group(3)
#     boundType = m.group(4)
#     
#     with open(path.join(aset,f), 'r') as fh:
#         for line in fh:
#             if not foundConfig:
#                 if "race_pct_profiler" in line:
#                         assert False and "Trying to tabulate a race detection file"
#                 if "sched_app" in line:
#                     if "delay_bound 1" in line:
#                         bounding="db"
#                     else:
#                         bounding="pb"
#                     m = re.search("pb_limit (\d)", line)
#                     bound = m.group(1)
#                     foundConfig=True
#             if "ERROR" in line:
#                 num_errors+=1
#             elif "Starting execution" in line:
#                 num_schedules+=1
#             elif "NO MORE EXECUTIONS" in line:
#                 exploredAll=True
#             #races
#     
#     if not name in tests:
#         tests[name] = {}
#     
#     ratio = 0
#     if num_schedules > 0:
#         ratio = num_errors / num_schedules
#     tests[name][paramsName] = [num_schedules, num_errors, ratio, "*" if exploredAll else ""]
    
    
    
    #print name +","+ bounding +","+ bound +","+ \
    #str(num_schedules) +","+ str(num_errors) +","+ str(exploredAll) \
    #    +","+f
            
# for testName, paramsToResults in tests.items().sort():
#     sys.stdout.write(testName)
#     sys.stdout.write(",")
#     #num schedules for db1
#     sys.stdout.write(paramsToResults["db1"][0])
#     sys.stdout.write(",")
    
    

        
    

