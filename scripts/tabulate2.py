#! /usr/bin/env ipython

import os
import re
from os import path
import sys
from random import shuffle
import random
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import math
import shutil
from matplotlib_venn import venn2, venn3, venn3_circles

import cPickle as pickle

from pylab import array

from matplotlib.transforms import IdentityTransform, Affine2D
from matplotlib.collections import PathCollection

matplotlib.rcParams['ps.useafm'] = True
matplotlib.rcParams['pdf.use14corefonts'] = True
matplotlib.rcParams['text.usetex'] = True
matplotlib.rcParams['font.size'] = 14.0


notFound = 100000

os.chdir(path.join(os.environ["ROOT"], "benchmarks"))
origdir = path.abspath(path.curdir)


class BenchmarkSearchResults(object):
    def __init__(self):
        self.numSchedules = 0
        self.numBuggy = 0
        self.numNewSchedules = 0
        self.numNewBuggySchedules = 0
        self.numBeforeBuggy = -1
        self.numNewBeforeBuggy = -1
        self.iterativeNumBeforeBuggy = -1
        self.iterativeNumBuggy = 0
        self.iterativeNumSchedules = 0
        self.numSchedulesInPrevious = 0
        self.hitLimit = True
        self.maxThreads = -1
        self.maxEnabledThreads = -1
        self.maxSchedulingPoints = 0
        self.maxSteps = 0
    def makeEmptyString(self):
        self.numSchedules = ""
        self.numBuggy = ""
        self.numNewSchedules = ""
        self.numNewBuggySchedules = ""
        self.numBeforeBuggy = ""
        self.numNewBeforeBuggy = ""
        self.iterativeNumBeforeBuggy = ""
        self.iterativeNumBuggy = ""
        self.numSchedulesInPrevious = ""
        self.iterativeNumSchedules = ""
        self.hitLimit = False
        self.maxThreads = ""
        
    def makeNotFound(self, schedLimit):
        self.numSchedules = schedLimit
        self.numBuggy = 0
        self.numNewSchedules = 0
        self.numNewBuggySchedules = 0
        self.numBeforeBuggy = schedLimit-1
        self.numNewBeforeBuggy = 0
        self.iterativeNumBeforeBuggy = schedLimit-1
        self.iterativeNumBuggy = 0
        self.iterativeNumSchedules = schedLimit-1
        self.numSchedulesInPrevious = 0
        self.hitLimit = True
        self.maxThreads = -1
    
class Benchmark(object):
    def __init__(self, name):
        self.name = name
        self.latex_name = name
        self.pbFiles = {}
        self.dbFiles = {}
        self.dfsFiles = {}
        self.randomFiles = {}
        ':type: dict[int, dict[int, str]]'
        
        self.pctFiles = {}
        self.pbRes = {}
        self.dbRes = {}
        self.dfsRes = {}
        self.randomRes = {}
        ':type: dict[int, BenchmarkSearchResults]'
        self.pctRes = {}
        ':type: dict[int, BenchmarkSearchResults]'
        
        self.pbBoundReached = 0
        self.pbIterativeNumBeforeBuggy = -1
        self.pbFirstBuggySearchHitLimit = False
        self.pbCountOfBuggy = -1
        
        self.dbBoundReached = 0
        self.dbIterativeNumBeforeBuggy = -1
        self.dbFirstBuggySearchHitLimit = False
        self.dbCountOfBuggy = -1
        
        self.maxThreads = 1
        self.maxEnabledThreads = 1
        self.maxSchedPoints = 0
        self.maxSteps = -1
        self.maxThreadsInput = -1

        self.pct_k = -1
        
        self.schedLimit = 100000

        self.mapleFound = '???'
        self.mapleScheds = '???'
        self.mapleTime = '???'
        
        self.predRandToBuggy = -1
        self.predPctToBuggy = dict()
    
    def buggyPbSummary(self):
        if self.pbCountOfBuggy == -1:
            return str(self.pbCountOfBuggy)+","+",,,"
        else:
            return str(self.pbCountOfBuggy)+","+self.pbRes[self.pbCountOfBuggy].stringSummary()
    
    def getPbBuggyRes(self):
        if self.pbCountOfBuggy == -1:
            res = BenchmarkSearchResults()
            res.makeNotFound(self.schedLimit)
            return res
        else:
            return self.pbRes[self.pbCountOfBuggy]
    def getDbBuggyRes(self):
        if self.dbCountOfBuggy == -1:
            res = BenchmarkSearchResults()
            res.makeNotFound(self.schedLimit)
            return res
        else:
            return self.dbRes[self.dbCountOfBuggy]

benchmarks = {}
sortedBenchmarks = []


def read_in_maple():
    os.chdir(origdir)
    with open("mapleRes.txt", 'r') as fh:
        for line in fh:
            line = line.strip()
            if line.startswith('#') or len(line) == 0:
                continue
            line = line.split(' ')
            name = line[0] + '--' + line[1]
            assert name in benchmarks
            b = benchmarks[name]
            assert isinstance(b, Benchmark)
            b.mapleFound = int(line[2]) != 0
            b.mapleScheds = line[3]
            b.mapleTime = line[4]


def read_in_buggy():
    os.chdir(origdir)
    with open("buggy.txt", 'r') as fh:
        for line in fh:
            line = line.strip()
            if line.startswith('#') or len(line) == 0:
                continue
            line = line.split(' ')
            name = line[0] + '--' + line[1]
            if name not in benchmarks:
                benchmarks[name] = Benchmark(name)
            b = benchmarks[name]
            assert isinstance(b, Benchmark)
            b.maxSteps = int(line[3])
            b.maxThreadsInput = int(line[2])



def list_get (l, idx, default):
    try:
        return l[idx]
    except IndexError:
        return default

def initBenchmarkFilenames():
    os.chdir("__results")
    for folder in os.listdir("."):
        if "use" in folder:
            for f in os.listdir(folder):
                #       0                  1                    2              3    4
                #2013-08-30-00-07-07 -- chess -- InterlockedWorkStealQueue -- db -- 0,    0,   100 --
                #                                                                   bound,seed,startIndex
                parts = f.split("--")
                name = parts[1]+"--"+parts[2]
                if name not in benchmarks:
                    benchmarks[name] = Benchmark(name)
                b = benchmarks[name]
                assert isinstance(b, Benchmark)
                boundParts = parts[4].split(",")
                bound = int(boundParts[0])

                startIndex = int(list_get(boundParts, 2, 0))

                if parts[3] == "pct":
                    mapIndexToFile = {}
                    if bound in b.pctFiles:
                        mapIndexToFile = b.pctFiles[bound]
                    else:
                        b.pctFiles[bound] = mapIndexToFile
                    assert startIndex not in mapIndexToFile, "pct benchmark: %s, %d" % (name, startIndex)
                    mapIndexToFile[startIndex] = path.join(folder, f)
                elif parts[3] == "random":
                    mapIndexToFile = {}
                    if bound in b.randomFiles:
                        mapIndexToFile = b.randomFiles[bound]
                    else:
                        b.randomFiles[bound] = mapIndexToFile
                    assert startIndex not in mapIndexToFile, "rand benchmark: %s, %d" % (f, startIndex)
                    mapIndexToFile[startIndex] = path.join(folder, f)
                    #assert bound not in b.randomFiles, "repeat bound %d for %s" % (bound, name)
                    #b.randomFiles[bound] = path.join(folder, f)
                elif parts[3] == "pb":
                    assert bound not in b.pbFiles, "repeat bound %d for %s" % (bound, name)
                    b.pbFiles[bound] = path.join(folder, f)
                elif parts[3] == "db":
                    assert bound not in b.dbFiles, "repeat bound %d for %s" % (bound, name)
                    b.dbFiles[bound] = path.join(folder, f)
                elif parts[3] == "dfs":
                    assert bound not in b.dfsFiles, "repeat bound %d for %s" % (bound, name)
                    b.dfsFiles[bound] = path.join(folder, f)

                else:
                    assert False


def debugPrint(s):
    print s

matcherThreads = re.compile(".*?HandleThreadStart (\d*)")
matcherEThreads = re.compile(".*?Max num enabled threads (\d*)")
matcherSchedPoints = re.compile(".*?Number of important scheduling points (\d*)")
matcherSteps = re.compile(".*?PCT NUM STEPS: (\d*)")

def getResultsRandom(filesMap):
    res = BenchmarkSearchResults()
    for(startIndex, f) in sorted(filesMap.items(), key=lambda k: int(k[0])):
        startIndex = int(startIndex)
        print "Start index=%d, numschedules=%d" % (startIndex, res.numSchedules)
        assert startIndex==res.numSchedules

        with open(f, 'r') as fh:
            foundErrorThisSchedule = False
            for line in fh:
                if "race_pct_profiler" in line:
                    assert False, "Trying to tabulate a race detection file "+f
                #elif "PCT NUM STEPS" in line:
                #    m = matcherSteps.match(line)
                #    res.maxSteps = max(res.maxSteps, int(m.group(1)))
                elif "finished execution" in line:
                    res.numSchedules += 1
                    if foundErrorThisSchedule:
                        res.numBuggy+=1
                        if res.numBeforeBuggy == -1:
                            res.numBeforeBuggy = res.numSchedules
                    foundErrorThisSchedule = False
                elif "ERROR" in line or "WARNING: rt->scriptFilenameTable is null" in line:
                    foundErrorThisSchedule = True
    return res


def getResultsPct(filesMap):
    res = BenchmarkSearchResults()
    for(startIndex, f) in sorted(filesMap.items(), key=lambda k: int(k[0])):
        startIndex = int(startIndex)
        print "Start index=%d, numschedules=%d" % (startIndex, res.numSchedules)
        assert startIndex==res.numSchedules

        with open(f, 'r') as fh:
            foundErrorThisSchedule = False
            for line in fh:
                if "race_pct_profiler" in line:
                    assert False, "Trying to tabulate a race detection file "+f
                elif "PCT NUM STEPS" in line:
                    m = matcherSteps.match(line)
                    res.maxSteps = max(res.maxSteps, int(m.group(1)))
                elif "finished execution" in line:
                    res.numSchedules += 1
                    if foundErrorThisSchedule:
                        res.numBuggy+=1
                        if res.numBeforeBuggy == -1:
                            res.numBeforeBuggy = res.numSchedules
                    foundErrorThisSchedule = False
                elif "ERROR" in line or "WARNING: rt->scriptFilenameTable is null" in line:
                    foundErrorThisSchedule = True
    return res

def getResults(filesMap, schedLimit):
    res = {}
    prevBenchmarkResult = BenchmarkSearchResults()
    lastBound = -1
    for (bound, file) in sorted(filesMap.items()):
        if lastBound+1 != bound:
            debugPrint("######### ERROR: Gap in benchmark "+file)
        lastBound+=1
        debugPrint("Searching %s with bound %d" % (file, bound))
        benchmarkResult = BenchmarkSearchResults()
        benchmarkResult.numSchedulesInPrevious = prevBenchmarkResult.numSchedules
        if prevBenchmarkResult.numSchedules >= schedLimit:
            break
        with open(file, 'r') as fh:
            foundErrorThisSchedule=False
            newSchedule = False
            for line in fh:
                if "race_pct_profiler" in line:
                    assert False, "Trying to tabulate a race detection file "+file
                elif "ERROR" in line or "WARNING: rt->scriptFilenameTable is null" in line:
                    foundErrorThisSchedule = True
                elif "Number of preemptions/delays:" in line:
                    boundCount = int(line.split(":")[1].strip())
                    if boundCount == bound:
                        newSchedule = True
                elif "Max num enabled threads" in line:
                    m = matcherEThreads.match(line)
                    benchmarkResult.maxEnabledThreads = max(benchmarkResult.maxEnabledThreads, int(m.group(1)))
                elif "HandleThreadStart" in line:
                    m = matcherThreads.match(line)
                    benchmarkResult.maxThreads = max(benchmarkResult.maxThreads, int(m.group(1)))
                elif "Number of important scheduling points" in line:
                    m = matcherSchedPoints.match(line)
                    benchmarkResult.maxSchedulingPoints = max(benchmarkResult.maxSchedulingPoints, int(m.group(1)))
                elif "NO MORE EXECUTIONS" in line:
                    benchmarkResult.hitLimit = False
                elif "finished execution" in line:
                    if newSchedule and benchmarkResult.iterativeNumSchedules+1>schedLimit:
                        break
                    benchmarkResult.numSchedules+=1
                    if newSchedule:
                        benchmarkResult.numNewSchedules+=1
                    if newSchedule and foundErrorThisSchedule:
                        benchmarkResult.numNewBuggySchedules+=1
                    benchmarkResult.iterativeNumSchedules = prevBenchmarkResult.numSchedules + benchmarkResult.numNewSchedules
                    if foundErrorThisSchedule:
                        benchmarkResult.numBuggy+=1
                        if benchmarkResult.numBeforeBuggy == -1:
                            benchmarkResult.numBeforeBuggy = benchmarkResult.numSchedules
                            benchmarkResult.numNewBeforeBuggy = benchmarkResult.numNewSchedules 
                        if benchmarkResult.iterativeNumBeforeBuggy == -1:
                            benchmarkResult.iterativeNumBeforeBuggy = benchmarkResult.numSchedulesInPrevious + benchmarkResult.numNewBeforeBuggy
                        benchmarkResult.iterativeNumBuggy = prevBenchmarkResult.iterativeNumBuggy + benchmarkResult.numNewBuggySchedules
                    foundErrorThisSchedule = False
                    newSchedule = False
        
        
        debugPrint("Found %d schedules of which %d buggy" % (benchmarkResult.numSchedules, benchmarkResult.numBuggy))
        
        res[bound] = benchmarkResult
        prevBenchmarkResult = benchmarkResult
    return res
                    

def getPredRandRes(normalRes, schedLimit):
    assert isinstance(normalRes, BenchmarkSearchResults)
    predToBuggy = -1
    if normalRes.numBuggy > 0:
        predToBuggy = float(schedLimit) / float(normalRes.numBuggy)
    return predToBuggy

def getRandomResultsIntoBenchmarks():
    for benchmark in benchmarks.values():
        assert isinstance(benchmark, Benchmark)
        debugPrint('Processing PCT result: '+benchmark.name)
        for(bound, files) in sorted(benchmark.pctFiles.items()):
            debugPrint('.. d = '+str(bound))
            assert bound not in benchmark.pctRes
            benchmark.pctRes[bound]=getResultsPct(files)
            assert benchmark.pctRes[bound].numSchedules == benchmark.schedLimit
            benchmark.predPctToBuggy[bound] = getPredRandRes(benchmark.pctRes[bound], benchmark.schedLimit)
            
        debugPrint('Processing Rand result: '+benchmark.name)
        assert len(benchmark.randomFiles) == 1
        assert len(benchmark.randomRes) == 0
        benchmark.randomRes[0] = getResultsRandom(benchmark.randomFiles[0])
        assert benchmark.randomRes[0].numSchedules == benchmark.schedLimit
        benchmark.predRandToBuggy = getPredRandRes(benchmark.randomRes[0], benchmark.schedLimit)
        

def getResultsIntoBenchmarks():
    for benchmark in benchmarks.values():
        assert isinstance(benchmark, Benchmark)
        benchmark.pbRes=getResults(benchmark.pbFiles, benchmark.schedLimit)
        benchmark.dbRes=getResults(benchmark.dbFiles, benchmark.schedLimit)
        benchmark.dfsRes=getResults(benchmark.dfsFiles, benchmark.schedLimit)
        #benchmark.randomRes=getResults(benchmark.randomFiles)

        
        for (bound, results) in sorted(benchmark.pbRes.items()):
            assert isinstance(results, BenchmarkSearchResults)
            if results.iterativeNumSchedules < benchmark.schedLimit:
                benchmark.pbBoundReached = bound+1
                benchmark.maxThreads = max(benchmark.maxThreads, results.maxThreads)
                benchmark.maxEnabledThreads = max(benchmark.maxEnabledThreads, results.maxEnabledThreads)
                benchmark.maxSchedPoints = max(benchmark.maxSchedPoints, results.maxSchedulingPoints)
            if results.iterativeNumBeforeBuggy != -1:
                if results.iterativeNumSchedules >= benchmark.schedLimit:
                    break
                benchmark.pbIterativeNumBeforeBuggy = results.iterativeNumBeforeBuggy
                benchmark.pbFirstBuggySearchHitLimit = results.hitLimit
                benchmark.pbCountOfBuggy = bound
                break

        for (bound, results) in sorted(benchmark.dbRes.items()):
            assert isinstance(results, BenchmarkSearchResults)
            if results.iterativeNumSchedules < benchmark.schedLimit:
                benchmark.dbBoundReached = bound+1
                benchmark.maxThreads = max(benchmark.maxThreads, results.maxThreads)
                benchmark.maxEnabledThreads = max(benchmark.maxEnabledThreads, results.maxEnabledThreads)
                benchmark.maxSchedPoints = max(benchmark.maxSchedPoints, results.maxSchedulingPoints)
            if results.iterativeNumBeforeBuggy != -1:
                if results.iterativeNumSchedules >= benchmark.schedLimit:
                    break
                benchmark.dbIterativeNumBeforeBuggy = results.iterativeNumBeforeBuggy
                benchmark.dbFirstBuggySearchHitLimit = results.hitLimit
                benchmark.dbCountOfBuggy = bound
                break

        for (bound, results) in sorted(benchmark.dfsRes.items()):
            assert isinstance(results, BenchmarkSearchResults)
            benchmark.maxThreads = max(benchmark.maxThreads, results.maxThreads)
            benchmark.maxEnabledThreads = max(benchmark.maxEnabledThreads, results.maxEnabledThreads)
            benchmark.maxSchedPoints = max(benchmark.maxSchedPoints, results.maxSchedulingPoints)
            


def createBenchmarkBin():
    read_in_buggy()
    read_in_maple()
    initBenchmarkFilenames()
    
    #del benchmarks['chess--InterlockedWorkStealQueueWithState']
    #del benchmarks['parsec-2.0--streamcluster']
    #del benchmarks['chess--StateWorkStealQueue']
    
    benchmarks['chess--InterlockedWorkStealQueueWithState'].schedLimit = 10000
    benchmarks['chess--StateWorkStealQueue'].schedLimit = 10000
    
    #for (name, benchmark) in sorted(benchmarks.items()):
    #    assert isinstance(benchmark, Benchmark)
    #    assert isinstance(name, str)
    #    print name
    #sys.exit(0)
    
    getResultsIntoBenchmarks()
    getRandomResultsIntoBenchmarks()
    os.chdir(origdir)
    with open("benchmarks.bin", 'wb') as fh:
        pickle.dump(benchmarks, fh, pickle.HIGHEST_PROTOCOL)

#print benchmarks

from matplotlib.ticker import MultipleLocator, FormatStrFormatter, FixedLocator

from matplotlib.ticker import Locator

class MinorSymLogLocator(Locator):
    
    """
    Dynamically find minor tick positions based on the positions of
    major ticks for a symlog scaling.
    """
    def __init__(self, linthresh=1.0):
        """
        Ticks will be placed between the major ticks.
        The placement is linear for x between -linthresh and linthresh,
        otherwise its logarithmically
        """
        self.linthresh = linthresh

    def __call__(self):
        import numpy as np
        'Return the locations of the ticks'
        majorlocs = self.axis.get_majorticklocs()

        # iterate through minor locs
        minorlocs = []

        # handle the lowest part
        for i in xrange(1, len(majorlocs)):
            majorstep = majorlocs[i] - majorlocs[i-1]
            if abs(majorlocs[i-1] + majorstep/2) < self.linthresh:
                ndivs = 10
            else:
                ndivs = 9
            minorstep = majorstep / ndivs
            locs = np.arange(majorlocs[i-1], majorlocs[i], minorstep)[1:]
            minorlocs.extend(locs)

        return self.raise_if_exceeds(np.array(minorlocs))

    def tick_values(self, vmin, vmax):
        raise NotImplementedError('Cannot get tick locations for a '
                                  '%s type.' % type(self))



def c_graph_preamble():
    fig, ax = plt.subplots(figsize=(8,6)) # figsize=(8,4) default: figsize=(8,6)
    
    
    
    
    majorLocator   = FixedLocator(range(0, len(benchmarks)+1, 10)) 
    #MultipleLocator(10)
    majorFormatter = FormatStrFormatter('%d')
    minorLocator   = MultipleLocator(1)
    
    
    ax.set_yscale('linear')
    #ax.set_xscale('log', nonposx='clip')
    ax.set_xscale('symlog')# linthreshx=10
    ax.set_xlim((0, notFound))
    ax.set_ylim((0, len(benchmarks)))
    ax.grid(True, clip_on=False, lw=0.3, alpha=0.4)
    ax.spines['left'].set_bounds(0, len(benchmarks))
    ax.spines['bottom'].set_bounds(0, notFound)
    
    #ax.set_xticks([1, 100, 1000, 10000, 100000])
    #ax.set_yticks(range(0,len(benchmarks)+1))
    
    ax.yaxis.set_major_locator(majorLocator)
    ax.yaxis.set_major_formatter(majorFormatter)
    
    #for the minor ticks, use no labels; default NullFormatter
    ax.yaxis.set_minor_locator(minorLocator)
    
    ax.xaxis.set_minor_locator(MinorSymLogLocator())#matplotlib.ticker.LogLocator())
    #ax.xaxis.set_minor_formatter(FormatStrFormatter('%d'))
    
    
    ax.set_xticklabels(['0', '1', '10', '100', '1000', '10000', '100000'])
    ax.set_yticklabels(['0', '10', '20', '30', '40'])
    plt.subplots_adjust(right=0.8)

    # diagonal
    #ax.plot([1, sched_limit], [1, sched_limit], ":", lw=1, c='k')

    # Hide all spines
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)
    
    #ax.spines['left'].set_visible(False)
    #ax.spines['bottom'].set_visible(False)
    
    # Only show ticks on the left and bottom spines
    ax.yaxis.set_ticks_position('left')
    ax.xaxis.set_ticks_position('bottom')

    ax.set_xlabel("AAAAAAAAAAAAAAAAAAAAAA")
    #, rotation="horizontal"
    ax.set_ylabel("AAAAAAAAAAAAAAAAAAAAAA")
    fig.tight_layout()
    plt.subplots_adjust(right=0.88)
    return fig, ax

def graphPreambleWoLog():
    fig, ax = plt.subplots()
    #ax.set_yscale('log')
    #ax.set_xscale('log')
    ax.set_xlim((0, notFound))
    ax.set_ylim((0, notFound))
    ax.grid(True, clip_on=False, lw=0.3, alpha=0.4)
    ax.spines['left'].set_bounds(0, notFound)
    ax.spines['bottom'].set_bounds(0, notFound)
    #ax.set_xticks([1, 100, 1000, 10000])
    #ax.set_yticks([1, 100, 1000, 10000])
    #ax.set_xticklabels(['1', '100', '1000', '10000'])
    #ax.set_yticklabels(['1', '100', '1000', '10000'])
    plt.subplots_adjust(right=0.8)
    
    #diagonal
    ax.plot([0,notFound], [0, notFound], ":", lw=1, c='k')
    
    # Hide all spines
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)
    #ax.spines['left'].set_visible(False)
    #ax.spines['bottom'].set_visible(False)
    # Only show ticks on the left and bottom spines
    ax.yaxis.set_ticks_position('left')
    ax.xaxis.set_ticks_position('bottom')
    
    ax.set_xlabel("\# terminal schedules (IDB)")
    #, rotation="horizontal"
    ax.set_ylabel("\# terminal schedules (IPB)")
    fig.tight_layout()
    return fig,ax


def graphPreamble():
    fig, ax = plt.subplots()
    ax.set_yscale('log')
    ax.set_xscale('log')
    ax.set_xlim((1, notFound))
    ax.set_ylim((1, notFound))
    ax.grid(True, clip_on=False, lw=0.3, alpha=0.4)
    ax.spines['left'].set_bounds(1, notFound)
    ax.spines['bottom'].set_bounds(1, notFound)
    ax.set_xticks([1, 100, 1000, 10000, 100000])
    ax.set_yticks([1, 100, 1000, 10000, 100000])
    ax.set_xticklabels(['1', '100', '1000', '10000', '100000'])
    ax.set_yticklabels(['1', '100', '1000', '10000', '100000'])
    plt.subplots_adjust(right=0.8)
    
    #diagonal
    ax.plot([1,notFound], [1, notFound], ":", lw=1, c='k')
    
    # Hide all spines
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)
    #ax.spines['left'].set_visible(False)
    #ax.spines['bottom'].set_visible(False)
    # Only show ticks on the left and bottom spines
    ax.yaxis.set_ticks_position('left')
    ax.xaxis.set_ticks_position('bottom')
    
    ax.set_xlabel("\# terminal schedules (IDB)")
    #, rotation="horizontal"
    ax.set_ylabel("\# terminal schedules (IPB)")
    fig.tight_layout()
    return fig,ax


def barPreamble(numBenchmarks):
    #figsize=(8,4)
    fig, ax = plt.subplots(figsize=(numBenchmarks*1.4+1.3,4)) # 16,4
    ax.set_yscale('log')
    #ax.set_yscale('symlog')
    ax.set_xscale('linear')

    ax.set_xlim((0, 1))
    ax.set_ylim((1, 100000))
    ax.grid(True, axis='y', clip_on=False, linestyle=':', lw=0.3, alpha=0.4, zorder=0)

    ax.grid(True, axis='x', linestyle=':', clip_on=False, lw=0.3, alpha=0.4)

    ax.spines['left'].set_bounds(1, 1000000)
    #ax.spines['bottom'].set_bounds(0, 1)

    ax.set_yticks([1, 10, 100, 1000, 10000, 100000, 1000000])
    ax.set_yticklabels(['', '1', '10', '100', '1000', '10000', '100000'])

    plt.subplots_adjust(right=0.8)

    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)

    ax.yaxis.set_ticks_position('left')
    ax.xaxis.set_ticks_position('bottom')


    plt.xlabel(' ')
    plt.ylabel(' ')

    plt.tick_params(\
    axis='x',          # changes apply to the x-axis
    which='both',      # both major and minor ticks are affected
    bottom='off',      # ticks along the bottom edge are off
    top='off',         # ticks along the top edge are off
    labelbottom='off') # labels along the bottom edge are off

    ax.set_xticks(range(6,200,6))

    return fig, ax


def barchart(filename, benchmarkSet):
    barBenchmarks = [
        #'CS.bluetooth_driver_bad',
        'CB.stringbuffer-jdk1.4',
        'parsec.ferret',
        'parsec.streamcluster',
        'parsec.streamcluster2',
        'parsec.streamcluster3',
        'chess.IWSQ',
        'chess.IWSQWS',
        'chess.SWSQ',
        'chess.WSQ',
        'radbench.bug1',
        'radbench.bug2',
        #'radbench.bug3',
        #'radbench.bug4',
        #'radbench.bug5',
        'radbench.bug6'
        ]


    if benchmarkSet == 0:
        barBenchmarks = set(barBenchmarks[0:5])
    else:
        barBenchmarks = set(barBenchmarks[5:13])
    
    print "Hello " + str(len(barBenchmarks))

    width=1
    gap=2

    group_size_est=4
    group_width_est=group_size_est*width


    



    fig, ax = barPreamble(len(barBenchmarks))

    plt.ylabel('\# buggy terminal schedules')

    ax.set_xlim((0, len(barBenchmarks)*(group_width_est+gap)))
    

    xpos=gap/2

    dark=0.1
    light=1

    

    
    grey=dark

    for idx,(oldname,benchmark) in enumerate(sortedBenchmarks):
        print benchmark.name
        if benchmark.name in barBenchmarks:
            grey=dark

            plt.annotate(
                benchmark.latex_name.replace('.', '.\n', 1).replace('buffer', 'buff'),
                (xpos + group_width_est/2,1),
                (0, -20),
                xycoords='data',
                textcoords='offset points',
                va='top',
                ha='center',
                rotation='0',
                size=10,
                weight='light',
                )

            print benchmark.name
            randomRes = benchmark.randomRes[0]
            assert isinstance(randomRes, BenchmarkSearchResults)


            plt.annotate(
                'rand',
                (float(xpos) + float(width)/2.0,1),
                (0, -2),
                xycoords='data',
                textcoords='offset points',
                va='top',
                ha='center',
                rotation='70',
                size=7,
                weight='light',
                )
            if randomRes.numBuggy >= 1:
                ax.bar(xpos, randomRes.numBuggy*10, width,
                       bottom=1,
                       alpha=1,
                       color=str(grey),
                       zorder=10)
            xpos += width

            grey=light

            for (d, pctRes) in benchmark.pctRes.iteritems():
                assert isinstance(pctRes, BenchmarkSearchResults)
                if d==0:
                    continue

                plt.annotate(
                    'd='+str(d),
                    (float(xpos) + float(width)/2.0,1),
                    (0, -2),
                    xycoords='data',
                    textcoords='offset points',
                    va='top',
                    ha='center',
                    rotation='70',
                    size=7,
                    weight='light',
                    )

                if pctRes.numBuggy >= 1:
                    ax.bar(xpos, pctRes.numBuggy*10, width,
                           bottom=1,
                           alpha=1,
                           color=str(grey),
                           zorder=10)
                xpos += width
                grey-=0.1
            xpos += gap

    fig.tight_layout(rect=(0,0.12,1,1))
    fig.savefig(filename)

def graph1(filename):
    fig, ax = graphPreamble()
    
    for idx,(name,benchmark) in enumerate(sortedBenchmarks):
        assert isinstance(benchmark, Benchmark)
        
        if benchmark.dbCountOfBuggy == -1 and benchmark.pbCountOfBuggy == -1:
            # No bug found by either technique
            continue
        
        dbBuggyRes = benchmark.getDbBuggyRes()
        pbBuggyRes = benchmark.getPbBuggyRes()
        assert isinstance(dbBuggyRes, BenchmarkSearchResults)
        assert isinstance(pbBuggyRes, BenchmarkSearchResults)
        
        x = [dbBuggyRes.iterativeNumBeforeBuggy,dbBuggyRes.numSchedules]
        y = [pbBuggyRes.iterativeNumBeforeBuggy,pbBuggyRes.numSchedules]
        print x,y
        
#         ax.plot(x,y,"-", lw=1.0, c='k', alpha=0.4, clip_on=False)
        ax.annotate("",
            xytext=(x[0], y[0]), textcoords='data',
            xy=(x[1], y[1]), xycoords='data',
            size=10,
            arrowprops=dict(arrowstyle="->",
                            linestyle='solid',
                            connectionstyle="arc3,rad=0.2",
                            lw=0.3, alpha=0.5)
            )
        a = ax.scatter(x[0], y[0],
                        s=50, lw=1, edgecolor="k", facecolor='none', marker='x', alpha=1, clip_on=False)
        ax.scatter(x[1], y[1],
                        s=10,
                        lw=0.5, edgecolor="k", facecolor='none', marker='s', alpha=0.9, clip_on=False)
        
#         if x[0] < 3:
#             a.set_paths((a.get_paths()[0].transformed(Affine2D().rotate_deg(idx*17)) ,))
        
        
        
        maxThreads = benchmark.dbRes[0].maxThreads
        #str(i)+" "+
        #str(maxThreads)
        #"(%d,%d)" % (benchmark.dbCountOfBuggy, benchmark.pbCountOfBuggy)
        temp = ax.text(x[1], y[1], str(idx),
                       size="8", ha="center", alpha=0.8)
        temp.set_transform(temp.get_transform() + Affine2D().translate(-6, 3))
    
    fig.savefig(filename, bbox_inches=0)


def graph2(filename):
    fig, ax = graphPreamble()
    
    for idx,(name,benchmark) in enumerate(sortedBenchmarks):
        assert isinstance(benchmark, Benchmark)
        
        if benchmark.dbCountOfBuggy == -1 and benchmark.pbCountOfBuggy == -1:
            # No bug found by either technique
            continue
        
        dbBuggyRes = benchmark.getDbBuggyRes()
        pbBuggyRes = benchmark.getPbBuggyRes()
        assert isinstance(dbBuggyRes, BenchmarkSearchResults)
        assert isinstance(pbBuggyRes, BenchmarkSearchResults)
        
        x = [max(dbBuggyRes.numSchedules - dbBuggyRes.iterativeNumBuggy,1),dbBuggyRes.numSchedules]
        y = [max(pbBuggyRes.numSchedules - pbBuggyRes.iterativeNumBuggy,1),pbBuggyRes.numSchedules]
        print benchmark.name
        print dbBuggyRes.iterativeNumBeforeBuggy, dbBuggyRes.numSchedules, dbBuggyRes.numBuggy
        
        assert dbBuggyRes.iterativeNumBuggy == dbBuggyRes.numBuggy
        assert pbBuggyRes.iterativeNumBuggy == pbBuggyRes.numBuggy
        
        assert dbBuggyRes.iterativeNumBeforeBuggy-1 <= dbBuggyRes.numSchedules - dbBuggyRes.numBuggy
        assert pbBuggyRes.iterativeNumBeforeBuggy-1 <= pbBuggyRes.numSchedules - pbBuggyRes.numBuggy
        
        assert x[0] != 0
        assert x[1] != 0
        assert y[0] != 0
        assert y[1] != 0
#         print x, y
#         distance = math.sqrt(
#                              math.pow(math.log10(x[1])-math.log10(x[0]),2) 
#                              + 
#                              math.pow(math.log10(y[1])-math.log10(y[0]),2)
#                              )
#         if distance < 100:
#             ax.plot(x,y,"-", lw=1.0, c='k', alpha=0.4, clip_on=False)
#         else:
        ax.annotate("",
            xytext=(x[0], y[0]), textcoords='data',
            xy=(x[1], y[1]), xycoords='data',
            size=10,
            arrowprops=dict(arrowstyle="->",
                            linestyle='solid',
                            connectionstyle="arc3,rad=0.2",
                            lw=0.5, alpha=0.5)
            )

        a = ax.scatter(x[0], y[0],
                        s=50, lw=1, edgecolor="k", facecolor='none', marker='x', alpha=1, clip_on=False)
        ax.scatter(x[1], y[1],
                        s=10,
                        lw=0.5, edgecolor="k", facecolor='none', marker='s', alpha=0.9, clip_on=False)
        
#         if x[0] < 3:
#             a.set_paths((a.get_paths()[0].transformed(Affine2D().rotate_deg(idx*17)) ,))
        
        
        
        maxThreads = benchmark.dbRes[0].maxThreads
        #str(i)+" "+
        #str(maxThreads)
        #"(%d,%d)" % (benchmark.dbCountOfBuggy, benchmark.pbCountOfBuggy)
        temp = ax.text(x[1], y[1], str(idx),
                       size="7", ha="center", alpha=0.8)
        temp.set_transform(temp.get_transform() + Affine2D().translate(-6, 3))
    
    fig.savefig(filename, bbox_inches=0)

def graph3(filename):
    fig, ax = graphPreamble()
    
    for idx,(name,benchmark) in enumerate(sortedBenchmarks):
        assert isinstance(benchmark, Benchmark)
        
        if benchmark.dbCountOfBuggy == -1 and benchmark.pbCountOfBuggy == -1:
            # No bug found by either technique
            continue
        
        dbBuggyRes = benchmark.getDbBuggyRes()
        pbBuggyRes = benchmark.getPbBuggyRes()
        assert isinstance(dbBuggyRes, BenchmarkSearchResults)
        assert isinstance(pbBuggyRes, BenchmarkSearchResults)
        
        x = [dbBuggyRes.iterativeNumBeforeBuggy,
             max(dbBuggyRes.numSchedules - dbBuggyRes.iterativeNumBuggy,1),
             dbBuggyRes.numSchedules]
        y = [pbBuggyRes.iterativeNumBeforeBuggy,
             max(pbBuggyRes.numSchedules - pbBuggyRes.iterativeNumBuggy,1),
             pbBuggyRes.numSchedules]
        
        print benchmark.name
        print dbBuggyRes.iterativeNumBeforeBuggy, dbBuggyRes.numSchedules, dbBuggyRes.numBuggy
        
        assert dbBuggyRes.iterativeNumBuggy == dbBuggyRes.numBuggy
        assert pbBuggyRes.iterativeNumBuggy == pbBuggyRes.numBuggy
        
        assert dbBuggyRes.iterativeNumBeforeBuggy-1 <= dbBuggyRes.numSchedules - dbBuggyRes.numBuggy
        assert pbBuggyRes.iterativeNumBeforeBuggy-1 <= pbBuggyRes.numSchedules - pbBuggyRes.numBuggy
        
        for xi in x:
            assert x != 0
        for yi in y:
            assert y != 0
#         print x, y
#         distance = math.sqrt(
#                              math.pow(math.log10(x[1])-math.log10(x[0]),2) 
#                              + 
#                              math.pow(math.log10(y[1])-math.log10(y[0]),2)
#                              )
#         if distance < 100:
#             ax.plot(x,y,"-", lw=1.0, c='k', alpha=0.4, clip_on=False)
#         else:
        ax.annotate("",
            xytext=(x[0], y[0]), textcoords='data',
            xy=(x[1], y[1]), xycoords='data',
            size=10,
            arrowprops=dict(arrowstyle="->",
                            linestyle='solid',
                            connectionstyle="arc3,rad=0.2",
                            lw=0.5, alpha=0.4)
            )
        ax.annotate("",
            xytext=(x[1], y[1]), textcoords='data',
            xy=(x[2], y[2]), xycoords='data',
            arrowprops=dict(arrowstyle="->",
                            linestyle='solid',
                            connectionstyle="arc3,rad=0.2",
                            lw=0.5, alpha=0.4)
            )

        a = ax.scatter(x[0], y[0],
                        s=50, lw=1, edgecolor="k", facecolor='none', marker='x', alpha=1, clip_on=False)
        ax.scatter(x[1], y[1],
                        s=10,
                        lw=0.5, edgecolor="k", facecolor='none', marker='o', alpha=0.5, clip_on=False)
        ax.scatter(x[2], y[2],
                        s=10,
                        lw=0.5, edgecolor="k", facecolor='none', marker='s', alpha=1, clip_on=False)
        
#         if x[0] < 3:
#             a.set_paths((a.get_paths()[0].transformed(Affine2D().rotate_deg(idx*17)) ,))
        
        
        
        maxThreads = benchmark.dbRes[0].maxThreads
        #str(i)+" "+
        #str(maxThreads)
        #"(%d,%d)" % (benchmark.dbCountOfBuggy, benchmark.pbCountOfBuggy)
        temp = ax.text(x[1], y[1], str(idx),
                       size="7", ha="center", alpha=0.5)
        temp.set_transform(temp.get_transform() + Affine2D().translate(-6, 3))
    
    fig.savefig(filename, bbox_inches=0)


def graph4(filename):
    fig, ax = graphPreamble()
    
    ax.set_xlabel("\# terminal schedules (IPB)")
    ax.set_ylabel("\# terminal schedules (DFS)")
    
    for idx,(name,benchmark) in enumerate(sortedBenchmarks):
        assert isinstance(benchmark, Benchmark)
        
        dfsRes = benchmark.dfsRes[0]
        assert isinstance(dfsRes, BenchmarkSearchResults)
        
        if dfsRes.numBuggy == 0 and benchmark.pbCountOfBuggy == -1:
            # No bug found by either technique
            continue
        
        pbBuggyRes = benchmark.getPbBuggyRes()
        assert isinstance(pbBuggyRes, BenchmarkSearchResults)
        
        x = [pbBuggyRes.iterativeNumBeforeBuggy, pbBuggyRes.numSchedules]
        y = [notFound if dfsRes.numBeforeBuggy == -1 else dfsRes.numBeforeBuggy,dfsRes.numSchedules]
        print benchmark.name
        print y
        
        assert pbBuggyRes.iterativeNumBuggy == pbBuggyRes.numBuggy
        
        assert dfsRes.iterativeNumBeforeBuggy-1 <= dfsRes.numSchedules - dfsRes.numBuggy
        assert pbBuggyRes.iterativeNumBeforeBuggy-1 <= pbBuggyRes.numSchedules - pbBuggyRes.numBuggy
        
        assert x[0] != 0
        assert x[1] != 0
        assert y[0] != 0
        assert y[1] != 0
#         print x, y
#         distance = math.sqrt(
#                              math.pow(math.log10(x[1])-math.log10(x[0]),2) 
#                              + 
#                              math.pow(math.log10(y[1])-math.log10(y[0]),2)
#                              )
#         if distance < 100:
        #ax.plot(x,y,"-", lw=0.5, c='k', alpha=0.5, clip_on=False)
#         else:
#         ax.annotate("",
#             xytext=(x[0], y[0]), textcoords='data',
#             xy=(x[1], y[1]), xycoords='data',
#             arrowprops=dict(arrowstyle="->",
#                             linestyle='solid',
#                             connectionstyle="arc3,rad=0.2",
#                             lw=0.5, alpha=0.5)
#             )

        a = ax.scatter(x[0], y[0],
                        s=50, lw=1, edgecolor="k", facecolor='none', marker='x', alpha=1, clip_on=False)
        #ax.scatter(x[1], y[1],
        #                s=10,
        #                lw=0.5, edgecolor="k", facecolor='none', marker='s', alpha=0.9, clip_on=False)
        
#         if x[0] < 3:
#             a.set_paths((a.get_paths()[0].transformed(Affine2D().rotate_deg(idx*17)) ,))
        
        
        
        maxThreads = benchmark.dbRes[0].maxThreads
        #str(i)+" "+
        #str(maxThreads)
        #"(%d,%d)" % (benchmark.dbCountOfBuggy, benchmark.pbCountOfBuggy)
        
        temp = ax.text(x[0], y[0], str(idx),
                       size="7", ha="center", alpha=0.8)
        temp.set_transform(temp.get_transform() + Affine2D().translate(-6, 3))
    
    fig.savefig(filename, bbox_inches=0)

def graph5(filename):
    fig, ax = graphPreamble()
    
    ax.set_xlabel("\# terminal schedules (IDB)")
    ax.set_ylabel("\# terminal schedules (DFS)")
    
    for idx,(name,benchmark) in enumerate(sortedBenchmarks):
        assert isinstance(benchmark, Benchmark)
        
        dfsRes = benchmark.dfsRes[0]
        assert isinstance(dfsRes, BenchmarkSearchResults)
        
        if dfsRes.numBuggy == 0 and benchmark.pbCountOfBuggy == -1:
            # No bug found by either technique
            continue
        
        dbBuggyRes = benchmark.getDbBuggyRes()
        assert isinstance(dbBuggyRes, BenchmarkSearchResults)
        
        x = [dbBuggyRes.iterativeNumBeforeBuggy, dbBuggyRes.numSchedules]
        y = [notFound if dfsRes.numBeforeBuggy == -1 else dfsRes.numBeforeBuggy,dfsRes.numSchedules]
        print benchmark.name
        print y
        
        assert dbBuggyRes.iterativeNumBuggy == dbBuggyRes.numBuggy
        
        assert dfsRes.iterativeNumBeforeBuggy-1 <= dfsRes.numSchedules - dfsRes.numBuggy
        assert dbBuggyRes.iterativeNumBeforeBuggy-1 <= dbBuggyRes.numSchedules - dbBuggyRes.numBuggy
        
        assert x[0] != 0
        assert x[1] != 0
        assert y[0] != 0
        assert y[1] != 0
#         print x, y
#         distance = math.sqrt(
#                              math.pow(math.log10(x[1])-math.log10(x[0]),2) 
#                              + 
#                              math.pow(math.log10(y[1])-math.log10(y[0]),2)
#                              )
#         if distance < 100:
        #ax.plot(x,y,"-", lw=0.5, c='k', alpha=0.5, clip_on=False)
#         else:
#         ax.annotate("",
#             xytext=(x[0], y[0]), textcoords='data',
#             xy=(x[1], y[1]), xycoords='data',
#             arrowprops=dict(arrowstyle="->",
#                             linestyle='solid',
#                             connectionstyle="arc3,rad=0.2",
#                             lw=0.5, alpha=0.5)
#             )

        a = ax.scatter(x[0], y[0],
                        s=50, lw=1, edgecolor="k", facecolor='none', marker='x', alpha=1, clip_on=False)
        #ax.scatter(x[1], y[1],
        #                s=10,
        #                lw=0.5, edgecolor="k", facecolor='none', marker='s', alpha=0.9, clip_on=False)
        
#         if x[0] < 3:
#             a.set_paths((a.get_paths()[0].transformed(Affine2D().rotate_deg(idx*17)) ,))
        
        
        
        maxThreads = benchmark.dbRes[0].maxThreads
        #str(i)+" "+
        #str(maxThreads)
        #"(%d,%d)" % (benchmark.dbCountOfBuggy, benchmark.pbCountOfBuggy)
        
        temp = ax.text(x[0], y[0], str(idx),
                       size="7", ha="center", alpha=0.8)
        temp.set_transform(temp.get_transform() + Affine2D().translate(-6, 3))
    
    fig.savefig(filename, bbox_inches=0)


def graphIdbVsDfs(filename):
    fig, ax = graphPreamble()
    
    ax.set_xlabel("\# terminal schedules (IDB)")
    ax.set_ylabel("\# terminal schedules (DFS)")
    
    for idx,(name,benchmark) in enumerate(sortedBenchmarks):
        assert isinstance(benchmark, Benchmark)
        
        dfsRes = benchmark.dfsRes[0]
        assert isinstance(dfsRes, BenchmarkSearchResults)
        
        if dfsRes.numBuggy == 0 and benchmark.dbCountOfBuggy == -1:
            # No bug found by either technique
            continue
        
        dbBuggyRes = benchmark.getDbBuggyRes()
        assert isinstance(dbBuggyRes, BenchmarkSearchResults)
        
        x = [notFound if benchmark.dbCountOfBuggy == -1 else dbBuggyRes.iterativeNumBeforeBuggy]
        y = [notFound if dfsRes.numBuggy == 0 else dfsRes.numBeforeBuggy]
        
        if x[0] == 0:
            x[0] = 1
        if y[0] == 0:
            y[0] = 1
        
        assert x[0] != 0
        assert y[0] != 0
#         print x, y
#         distance = math.sqrt(
#                              math.pow(math.log10(x[1])-math.log10(x[0]),2) 
#                              + 
#                              math.pow(math.log10(y[1])-math.log10(y[0]),2)
#                              )
#         if distance < 100:
        #ax.plot(x,y,"-", lw=0.5, c='k', alpha=0.5, clip_on=False)
#         else:
#         ax.annotate("",
#             xytext=(x[0], y[0]), textcoords='data',
#             xy=(x[1], y[1]), xycoords='data',
#             arrowprops=dict(arrowstyle="->",
#                             linestyle='solid',
#                             connectionstyle="arc3,rad=0.2",
#                             lw=0.5, alpha=0.5)
#             )

        a = ax.scatter(x[0], y[0],
                        s=50, lw=1, edgecolor="k", facecolor='none', marker='x', alpha=1, clip_on=False)
        #ax.scatter(x[1], y[1],
        #                s=10,
        #                lw=0.5, edgecolor="k", facecolor='none', marker='s', alpha=0.9, clip_on=False)
        
#         if x[0] < 3:
#             a.set_paths((a.get_paths()[0].transformed(Affine2D().rotate_deg(idx*17)) ,))
        
        
        
        maxThreads = benchmark.dbRes[0].maxThreads
        #str(i)+" "+
        #str(maxThreads)
        #"(%d,%d)" % (benchmark.dbCountOfBuggy, benchmark.pbCountOfBuggy)
        
        temp = ax.text(x[0], y[0], " "+str(idx), size="7", ha="center", alpha=0.8)
        temp.set_transform(temp.get_transform() + Affine2D().translate(-6, 3))
    
    fig.savefig(filename, bbox_inches=0)


def graphWorstIdbVsWorstRandom(filename):
    fig, ax = graphPreamble()
    
    ax.set_xlabel("\# terminal schedules (IDB)")
    ax.set_ylabel("\# terminal schedules (Rand)")
    
    for idx,(name,benchmark) in enumerate(sortedBenchmarks):
        assert isinstance(benchmark, Benchmark)
        
        randRes = benchmark.randomRes[0]
        assert isinstance(randRes, BenchmarkSearchResults)
        
        if randRes.numBuggy == 0 and benchmark.dbCountOfBuggy == -1:
            # No bug found by either technique
            continue
        
        dbBuggyRes = benchmark.getDbBuggyRes()
        assert isinstance(dbBuggyRes, BenchmarkSearchResults)
        
        x = [max(dbBuggyRes.numSchedules - dbBuggyRes.iterativeNumBuggy,1)]
        y = [max(notFound - randRes.numBuggy,1)]
        
        if x[0] == 0:
            x[0] = 1
        if y[0] == 0:
            y[0] = 1
        
        assert x[0] != 0
        assert y[0] != 0
#         print x, y
#         distance = math.sqrt(
#                              math.pow(math.log10(x[1])-math.log10(x[0]),2) 
#                              + 
#                              math.pow(math.log10(y[1])-math.log10(y[0]),2)
#                              )
#         if distance < 100:
        #ax.plot(x,y,"-", lw=0.5, c='k', alpha=0.5, clip_on=False)
#         else:
#         ax.annotate("",
#             xytext=(x[0], y[0]), textcoords='data',
#             xy=(x[1], y[1]), xycoords='data',
#             arrowprops=dict(arrowstyle="->",
#                             linestyle='solid',
#                             connectionstyle="arc3,rad=0.2",
#                             lw=0.5, alpha=0.5)
#             )

        a = ax.scatter(x[0], y[0],
                        s=50, lw=1, edgecolor="k", facecolor='none', marker='x', alpha=1, clip_on=False)
        #ax.scatter(x[1], y[1],
        #                s=10,
        #                lw=0.5, edgecolor="k", facecolor='none', marker='s', alpha=0.9, clip_on=False)
        
#         if x[0] < 3:
#             a.set_paths((a.get_paths()[0].transformed(Affine2D().rotate_deg(idx*17)) ,))
        
        
        
        maxThreads = benchmark.dbRes[0].maxThreads
        #str(i)+" "+
        #str(maxThreads)
        #"(%d,%d)" % (benchmark.dbCountOfBuggy, benchmark.pbCountOfBuggy)
        
        temp = ax.text(x[0], y[0], " "+str(idx), size="7", ha="center", alpha=0.8)
        temp.set_transform(temp.get_transform() + Affine2D().translate(-6, 3))
    
    fig.savefig(filename, bbox_inches=0)

x_places = [i for i in range(0, notFound + 1, 1)]

cumulative_pb = [0 for i in x_places]
cumulative_db = [0 for i in x_places]
cumulative_dfs = [0 for i in x_places]

cumulative_rand = [0 for i in x_places]
cumulative_pct1 = [0 for i in x_places]
cumulative_pct2 = [0 for i in x_places]
cumulative_pct3 = [0 for i in x_places]

cumulative_randpred = [0 for i in x_places]
cumulative_pct1pred = [0 for i in x_places]
cumulative_pct2pred = [0 for i in x_places]
cumulative_pct3pred = [0 for i in x_places]

def calc_cumulative():
    
    exclude = []
#                "CS.wronglock_3_bad",
#                "CS.twostage_100_bad",
#                "CS.reorder_10_bad",
#                "CS.reorder_20_bad",
#                "CS.reorder_4_bad",
#                "CS.reorder_5_bad",
#                "CS.din_phil7_sat",
#                "CS.din_phil6_sat",
#                "CS.din_phil5_sat",
#                "CS.din_phil4_sat",
#                "CS.din_phil3_sat"
#                ]
    
    for (i,x) in enumerate(x_places):
        for bench in benchmarks.values():
            assert isinstance(bench, Benchmark)
            if bench.name in exclude:
                continue
            
            dfsRes = bench.dfsRes[0]
            assert isinstance(dfsRes, BenchmarkSearchResults)
            
            if bench.pbIterativeNumBeforeBuggy <= x and bench.pbIterativeNumBeforeBuggy != -1:
                cumulative_pb[i] += 1
            if bench.dbIterativeNumBeforeBuggy <= x and bench.dbIterativeNumBeforeBuggy != -1:
                cumulative_db[i] += 1
            if dfsRes.numBeforeBuggy <= x and dfsRes.numBuggy != 0:
                cumulative_dfs[i] += 1
            
            
            if bench.randomRes[0].numBeforeBuggy <=x and bench.randomRes[0].numBuggy != 0:
                cumulative_rand[i] += 1
            
            if bench.pctRes[1].numBeforeBuggy <= x and bench.pctRes[1].numBuggy !=0:
                cumulative_pct1[i] += 1
            
            if bench.pctRes[2].numBeforeBuggy <= x and bench.pctRes[2].numBuggy !=0:
                cumulative_pct2[i] += 1
            
            if bench.pctRes[3].numBeforeBuggy <= x and bench.pctRes[3].numBuggy !=0:
                cumulative_pct3[i] += 1
            
            if bench.predPctToBuggy[1] <= x and bench.predPctToBuggy[1] >= 0:
                cumulative_pct1pred[i] += 1
            
            if bench.predPctToBuggy[2] <= x and bench.predPctToBuggy[2] >= 0:
                cumulative_pct2pred[i] += 1
            
            if bench.predPctToBuggy[3] <= x and bench.predPctToBuggy[3] >= 0:
                cumulative_pct3pred[i] += 1
            
            if bench.predRandToBuggy <= x and bench.predRandToBuggy >= 0:
                cumulative_randpred[i] += 1


cg_label = "%s (%d)"

def graph_cumulative(filename, benchmarks):
    """
    :type benchmarks_sorted: list of str
    :type benchmarks: dict[str, Benchmark]

    :param filename:
    :param benchmarks_sorted:
    """
    fig, ax = c_graph_preamble()
    
    ax.set_xlabel("\# terminal schedules")
    ax.set_ylabel("\# bugs found")
    
    
    
    
    #cumulative[0] = 1
    #cumulative_l[0] = 1
    
    
    #darkorange
    #dodgerblue
    #
    #
    plt.plot(x_places, cumulative_pb, c='dodgerblue', linestyle='-', alpha=0.6)
    temp = ax.text(x_places[-1], cumulative_pb[-1], cg_label % ("IPB", cumulative_pb[-1]),
                       size="10", ha="left", alpha=1)
    temp.set_transform(temp.get_transform() + Affine2D().translate(+3, 0))
    
#     temp = ax.text(x_places[-1], cumulative_pb[-1], str(cumulative_pb[-1]),
#                        size="10", ha="left", alpha=1)
#     temp.set_transform(temp.get_transform() + Affine2D().translate(+3+xcoord_cg, 0))
    
    
    plt.plot(x_places, cumulative_rand, c='gold', alpha=1)
    temp = ax.text(x_places[-1], cumulative_rand[-1], cg_label % ("Rand", cumulative_rand[-1]),
                       size="10", ha="left", alpha=1)
    temp.set_transform(temp.get_transform() + Affine2D().translate(+3, -4))
    
    
    plt.plot(x_places, cumulative_db, c='darkorange',
                      dashes=[4, 1],
                      linestyle='-', alpha=1)
    temp = ax.text(x_places[-1], cumulative_db[-1], cg_label % ("IDB", cumulative_db[-1]),
                       size="10", ha="left", alpha=1)
    temp.set_transform(temp.get_transform() + Affine2D().translate(+3, -4))
    
    
    
    plt.plot(x_places, cumulative_dfs, c='black', dashes=[3, 3], linestyle='-')
    temp = ax.text(x_places[-1], cumulative_dfs[-1],  cg_label % ("DFS", cumulative_dfs[-1]),
                       size="10", ha="left", alpha=1)
    temp.set_transform(temp.get_transform() + Affine2D().translate(+3, 0))
    
    
    plt.plot(x_places, cumulative_pct1, c='black', alpha=0.2)
    
    temp = ax.text(x_places[-1], cumulative_pct1[-1],  cg_label % ("PCT d=1", cumulative_pct1[-1]),
                       size="10", ha="left", alpha=1)
    temp.set_transform(temp.get_transform() + Affine2D().translate(+3, 0))
    
    
    plt.plot(x_places, cumulative_pct2, c='black', alpha=0.4)
    
    temp = ax.text(x_places[-1], cumulative_pct2[-1], cg_label % ("PCT d=2", cumulative_pct2[-1]),
                       size="10", ha="left", alpha=1)
    temp.set_transform(temp.get_transform() + Affine2D().translate(+3, 0))
    
    plt.plot(x_places, cumulative_pct3, c='black', linewidth=1.1, alpha=1)
    
    temp = ax.text(x_places[-1], cumulative_pct3[-1], cg_label % ("PCT d=3", cumulative_pct3[-1]),
                       size="10", ha="left", alpha=1)
    temp.set_transform(temp.get_transform() + Affine2D().translate(+3, 0))
    
    
    
#     plt.plot(x_places, cumulative_pct3pred, c='black', dashes=[3, 3], linewidth=1, alpha=1)
#     
#     temp = ax.text(x_places[-1], cumulative_pct3pred[-1], "PCT d=3 (predicted)",
#                        size="10", ha="left", alpha=1)
#     temp.set_transform(temp.get_transform() + Affine2D().translate(+3, 0))
#     
#     
#     plt.plot(x_places, cumulative_randpred, c='gold', dashes=[4, 3], alpha=1)
#     temp = ax.text(x_places[-1], cumulative_rand[-1], "Rand (predicted)",
#                        size="10", ha="left", alpha=1)
#     temp.set_transform(temp.get_transform() + Affine2D().translate(+3, -4))
    
    

    # temp = ax.text(x[0], y[0], " "+str(idx), size="7", ha="center", alpha=0.8)
    #temp.set_transform(temp.get_transform() + Affine2D().translate(-6, 3))

    fig.savefig(filename, bbox_inches=0)


def graph_cumulative2(filename, benchmarks):
    """
    :type benchmarks_sorted: list of str
    :type benchmarks: dict[str, Benchmark]

    :param filename:
    :param benchmarks_sorted:
    """
    fig, ax = c_graph_preamble()
    
    ax.set_xlabel("\# terminal schedules")
    ax.set_ylabel("\# bugs found")
    
    
    
    
    #cumulative[0] = 1
    #cumulative_l[0] = 1
    
    
    #darkorange
    #dodgerblue
    #
    #
    
#     plt.plot(x_places, cumulative_pb, c='dodgerblue', linestyle='-', alpha=0.6)
#     temp = ax.text(x_places[-1], cumulative_pb[-1], "IPB",
#                        size="10", ha="left", alpha=1)
#     temp.set_transform(temp.get_transform() + Affine2D().translate(+3, 0))
    
    
    plt.plot(x_places, cumulative_rand, c='gold', alpha=1)
    temp = ax.text(x_places[-1], cumulative_rand[-1], cg_label % ("Rand", cumulative_rand[-1]),
                       size="10", ha="left", alpha=1)
    temp.set_transform(temp.get_transform() + Affine2D().translate(+3, -4))
    
    
#     plt.plot(x_places, cumulative_db, c='darkorange',
#                       dashes=[4, 1],
#                       linestyle='-', alpha=1)
#     temp = ax.text(x_places[-1], cumulative_db[-1], "IDB",
#                        size="10", ha="left", alpha=1)
#     temp.set_transform(temp.get_transform() + Affine2D().translate(+3, -4))
    
#     plt.plot(x_places, cumulative_dfs, c='black', dashes=[3, 3], linestyle='-')
#     temp = ax.text(x_places[-1], cumulative_dfs[-1], "DFS",
#                        size="10", ha="left", alpha=1)
#     temp.set_transform(temp.get_transform() + Affine2D().translate(+3, 0))
    
    
#     plt.plot(x_places, cumulative_pct1, c='black', alpha=0.2)
#      
#     temp = ax.text(x_places[-1], cumulative_pct1[-1], "PCT d=1",
#                        size="10", ha="left", alpha=1)
#     temp.set_transform(temp.get_transform() + Affine2D().translate(+3, 0))
#     
#     
#     plt.plot(x_places, cumulative_pct2, c='black', alpha=0.4)
#      
#     temp = ax.text(x_places[-1], cumulative_pct2[-1], "PCT d=2",
#                        size="10", ha="left", alpha=1)
#     temp.set_transform(temp.get_transform() + Affine2D().translate(+3, 0))
    
    plt.plot(x_places, cumulative_pct3, c='black', linewidth=1.1, alpha=1)
    
    temp = ax.text(x_places[-1], cumulative_pct3[-1], cg_label % ("PCT d=3", cumulative_pct3[-1]),
                       size="10", ha="left", alpha=1)
    temp.set_transform(temp.get_transform() + Affine2D().translate(+3, 0))
    
    
    
#     plt.plot(x_places, cumulative_pct1pred, c='black', dashes=[3, 2], alpha=0.2)
#     plt.plot(x_places, cumulative_pct2pred, c='black', dashes=[2, 2], alpha=0.4)
    plt.plot(x_places, cumulative_pct3pred, c='black', dashes=[3, 3], linewidth=1, alpha=1)
    
    plt.plot(x_places, cumulative_randpred, c='gold', dashes=[4, 3], alpha=1)
    
    

    # temp = ax.text(x[0], y[0], " "+str(idx), size="7", ha="center", alpha=0.8)
    #temp.set_transform(temp.get_transform() + Affine2D().translate(-6, 3))

    fig.savefig(filename, bbox_inches=0)

def graph_cumulative3(filename, benchmarks):
    """
    :type benchmarks_sorted: list of str
    :type benchmarks: dict[str, Benchmark]

    :param filename:
    :param benchmarks_sorted:
    """
    fig, ax = c_graph_preamble()
    
    ax.set_xlabel("\# terminal schedules")
    ax.set_ylabel("\# bugs found")
    
    
    
    
    #cumulative[0] = 1
    #cumulative_l[0] = 1
    
    
    #darkorange
    #dodgerblue
    #
    #
    plt.plot(x_places, cumulative_pb, c='dodgerblue', linestyle='-', alpha=0.6)
    temp = ax.text(x_places[-1], cumulative_pb[-1], cg_label % ("IPB", cumulative_pb[-1]),
                       size="10", ha="left", alpha=1)
    temp.set_transform(temp.get_transform() + Affine2D().translate(+3, 0))
    
    
    plt.plot(x_places, cumulative_randpred, c='gold', alpha=1)
    temp = ax.text(x_places[-1], cumulative_randpred[-1], cg_label % ("Rand *", cumulative_randpred[-1]),
                       size="10", ha="left", alpha=1)
    temp.set_transform(temp.get_transform() + Affine2D().translate(+3, -4))
    
    
    plt.plot(x_places, cumulative_db, c='darkorange',
                      dashes=[4, 1],
                      linestyle='-', alpha=1)
    temp = ax.text(x_places[-1], cumulative_db[-1], cg_label % ("IDB", cumulative_db[-1]),
                       size="10", ha="left", alpha=1)
    temp.set_transform(temp.get_transform() + Affine2D().translate(+3, -4))
    
    plt.plot(x_places, cumulative_dfs, c='black', dashes=[3, 3], linestyle='-')
    temp = ax.text(x_places[-1], cumulative_dfs[-1], cg_label % ("DFS", cumulative_dfs[-1]),
                       size="10", ha="left", alpha=1)
    temp.set_transform(temp.get_transform() + Affine2D().translate(+3, 0))
    
    
    plt.plot(x_places, cumulative_pct1pred, c='black', alpha=0.2)
    
    temp = ax.text(x_places[-1], cumulative_pct1pred[-1], cg_label % ("PCT d=1 *", cumulative_pct1pred[-1]),
                       size="10", ha="left", alpha=1)
    temp.set_transform(temp.get_transform() + Affine2D().translate(+3, 0))
    
    
    plt.plot(x_places, cumulative_pct2pred, c='black', alpha=0.4)
    
    temp = ax.text(x_places[-1], cumulative_pct2pred[-1], cg_label % ("PCT d=2 *", cumulative_pct2pred[-1]),
                       size="10", ha="left", alpha=1)
    temp.set_transform(temp.get_transform() + Affine2D().translate(+3, 0))
    
    plt.plot(x_places, cumulative_pct3pred, c='black', linewidth=1.1, alpha=1)
    
    temp = ax.text(x_places[-1], cumulative_pct3pred[-1], cg_label % ("PCT d=3 *", cumulative_pct3pred[-1]),
                       size="10", ha="left", alpha=1)
    temp.set_transform(temp.get_transform() + Affine2D().translate(+3, 0))
    
    
    
#     plt.plot(x_places, cumulative_pct3pred, c='black', dashes=[3, 3], linewidth=1, alpha=1)
#     
#     temp = ax.text(x_places[-1], cumulative_pct3pred[-1], "PCT d=3 (predicted)",
#                        size="10", ha="left", alpha=1)
#     temp.set_transform(temp.get_transform() + Affine2D().translate(+3, 0))
#     
#     
#     plt.plot(x_places, cumulative_randpred, c='gold', dashes=[4, 3], alpha=1)
#     temp = ax.text(x_places[-1], cumulative_rand[-1], "Rand (predicted)",
#                        size="10", ha="left", alpha=1)
#     temp.set_transform(temp.get_transform() + Affine2D().translate(+3, -4))
    
    

    # temp = ax.text(x[0], y[0], " "+str(idx), size="7", ha="center", alpha=0.8)
    #temp.set_transform(temp.get_transform() + Affine2D().translate(-6, 3))

    fig.savefig(filename, bbox_inches=0)

def venn_diagram(filename, benchmarkSets, labels):

    none = set()

    # populate "none" with the benchmarks that are missing
    for idx, (sn,benchmark) in enumerate(sortedBenchmarks):
        for bs in benchmarkSets:
            inNone = True
            if benchmark.name in bs:
                inNone = False
                break
        if inNone:
            none.add(benchmark.name)

    # Compute the union of all elements
    union = benchmarkSets[0].union(benchmarkSets[1]).union(benchmarkSets[2])

    # For each element compute its 'indicator'
    # (e.g. an indicator of 110 means element belongs to set1 and set2 but not set3)
    indicators = ['%d%d%d' % (a in benchmarkSets[0], a in benchmarkSets[1], a in benchmarkSets[2]) for a in union]

    # Use the standard Counter object (Python 2.7+) to count the frequency for each indicator
    from collections import Counter
    subsets = Counter(indicators)

    #print subsets

    plt.figure(figsize=(4,4))
    # Provide the resulting dictionary as the subsets parameter to venn3:
    v = venn3(subsets, set_labels=labels, set_colors=('r', 'g', '#2C58E8'), alpha=0.4, ax=None)
    plt.text(0.7, -0.2, str(len(none)), va='center', ha='center')
    plt.savefig(filename, transparent=True, bbox_inches='tight')


def venn_diagram_random(filename):

    pcta = set()
    pctb = set()
    rand = set()

    for idx, (sn,benchmark) in enumerate(sortedBenchmarks):
        assert isinstance(benchmark, Benchmark)
        if benchmark.pctRes[2].numBuggy != 0:
            pcta.add(benchmark.name)
        if benchmark.pctRes[3].numBuggy != 0:
            pctb.add(benchmark.name)
        if benchmark.randomRes[0].numBuggy != 0:
            rand.add(benchmark.name)


    venn_diagram(filename, (pcta, pctb, rand), ('PCT d=2', 'PCT d=3', 'Rand'))

def venn_diagram_pct_rand_idb(filename):

    pcta = set()
    rand = set()
    idb = set()

    for idx, (sn,benchmark) in enumerate(sortedBenchmarks):
        assert isinstance(benchmark, Benchmark)
        if benchmark.pctRes[3].numBuggy != 0:
            pcta.add(benchmark.name)
        if benchmark.dbCountOfBuggy != -1:
            idb.add(benchmark.name)
        if benchmark.randomRes[0].numBuggy != 0:
            rand.add(benchmark.name)


    venn_diagram(filename, (idb, rand, pcta), ('IDB', 'Rand', 'PCT d=3'))


def venn_diagram_pct_rand_ipb(filename):

    pcta = set()
    rand = set()
    ipb = set()

    for idx, (sn,benchmark) in enumerate(sortedBenchmarks):
        assert isinstance(benchmark, Benchmark)
        if benchmark.pctRes[3].numBuggy != 0:
            pcta.add(benchmark.name)
        if benchmark.pbCountOfBuggy != -1:
            ipb.add(benchmark.name)
        if benchmark.randomRes[0].numBuggy != 0:
            rand.add(benchmark.name)


    venn_diagram(filename, (ipb, rand, pcta), ('IDB', 'Rand', 'PCT d=3'))

def venn_diagram_pct_maple_idb(filename):

    pcta = set()
    pctb = set()
    maple = set()

    for idx, (sn,benchmark) in enumerate(sortedBenchmarks):
        assert isinstance(benchmark, Benchmark)
        if benchmark.pctRes[3].numBuggy != 0:
            pcta.add(benchmark.name)
        if benchmark.pctRes[2].numBuggy != 0:
            pctb.add(benchmark.name)
        if benchmark.mapleFound:
            maple.add(benchmark.name)


    venn_diagram(filename, (pctb, pcta, maple), ('PCT d=2', 'PCT d=3', 'MapleAlg'))

def venn_diagram1(filename):
    
    
    ipb = set()
    idb = set()
    dfs = set()
    
    none = set()
    
    for idx, (sn,benchmark) in enumerate(sortedBenchmarks):
            assert isinstance(benchmark, Benchmark)
            pbr = benchmark.getPbBuggyRes()
            dbr = benchmark.getDbBuggyRes()
            dfsr = benchmark.dfsRes[0]
            assert isinstance(pbr, BenchmarkSearchResults)
            assert isinstance(dbr, BenchmarkSearchResults)
            assert isinstance(dfsr, BenchmarkSearchResults)
            if benchmark.dbCountOfBuggy != -1:
                idb.add(benchmark.name)
            
            if benchmark.pbCountOfBuggy != -1:
                ipb.add(benchmark.name)
            
            if dfsr.numBuggy != 0:
                dfs.add(benchmark.name)
            
            if benchmark.dbCountOfBuggy == -1 and \
               benchmark.pbCountOfBuggy == -1 and \
               dfsr.numBuggy == 0:
                none.add(benchmark.name)
    
    # Compute the union of all elements
    union = ipb.union(idb).union(dfs)
    
    # For each element compute its 'indicator'
    # (e.g. an indicator of 110 means element belongs to set1 and set2 but not set3)
    indicators = ['%d%d%d' % (a in ipb, a in idb, a in dfs) for a in union]
    
    # Use the standard Counter object (Python 2.7+) to count the frequency for each indicator
    from collections import Counter
    subsets = Counter(indicators)
    
    print subsets
    
    plt.figure(figsize=(4,4))
    # Provide the resulting dictionary as the subsets parameter to venn3:
    v = venn3(subsets, set_labels=('IPB', 'IDB', 'DFS'), set_colors=('r', 'g', '#2C58E8'), alpha=0.4, ax=None)
    plt.text(0.7, -0.2, str(len(none)), va='center', ha='center')
    plt.savefig(filename, transparent=True, bbox_inches='tight')


def venn_diagram2(filename):
    
    
    #dfs = set()
    idb = set()
    rand = set()
    
    none = set()
    
    for idx, (sn,benchmark) in enumerate(sortedBenchmarks):
            assert isinstance(benchmark, Benchmark)
            dfsr = benchmark.dfsRes[0]
            dbr = benchmark.getDbBuggyRes()
            randr = benchmark.randomRes[0]
            assert isinstance(dfsr, BenchmarkSearchResults)
            assert isinstance(dbr, BenchmarkSearchResults)
            assert isinstance(randr, BenchmarkSearchResults)
            if benchmark.dbCountOfBuggy != -1:
                idb.add(benchmark.name)
            
            #if dfsr.numBuggy != 0:
            #    dfs.add(benchmark.name)
            
            if randr.numBuggy != 0:
                rand.add(benchmark.name)
            
            if benchmark.dbCountOfBuggy == -1 and \
               randr.numBuggy == 0:
                none.add(benchmark.name)
    
    # Compute the union of all elements
    union = idb.union(rand)
    
    # For each element compute its 'indicator'
    # (e.g. an indicator of 110 means element belongs to set1 and set2 but not set3)
    indicators = ['%d%d' % (a in idb, a in rand) for a in union]
    
    # Use the standard Counter object (Python 2.7+) to count the frequency for each indicator
    from collections import Counter
    subsets = Counter(indicators)
    
    print subsets
    
    plt.figure(figsize=(4,4))
    # Provide the resulting dictionary as the subsets parameter to venn3:
    v = venn2(subsets, set_labels=('IDB', 'Rand'), set_colors=('r', 'g', '#2C58E8'), alpha=0.4, ax=None)
    plt.text(0.7, -0.2, str(len(none)), va='center', ha='center')
    plt.savefig(filename, transparent=True, bbox_inches='tight')

def venn_diagram3(filename):
    
    mapleRes = {0:1,1:1,2:1,3:1,4:1,5:0,6:1,7:0,8:0,9:1,10:1,
11:1,12:1,13:1,14:1,15:1,16:1,17:1,18:1,19:0,
20:0,21:0,22:0,23:0,24:0,25:1,26:1,27:1,28:0,
29:1,30:1,31:1,32:0,33:0,34:0,35:0,36:0,37:1,
38:0,39:1,40:1,41:0,42:1,43:0,44:0,45:1,46:1,
47:1,48:0,49:1,50:1,51:1}
    
    idb = set()
    maple = set()
    rand = set()
    
    none = set()
    
    for idx, (sn,benchmark) in enumerate(sortedBenchmarks):
            assert isinstance(benchmark, Benchmark)
            dfsr = benchmark.dfsRes[0]
            dbr = benchmark.getDbBuggyRes()
            randr = benchmark.randomRes[0]
            assert isinstance(dfsr, BenchmarkSearchResults)
            assert isinstance(dbr, BenchmarkSearchResults)
            assert isinstance(randr, BenchmarkSearchResults)
            
            if benchmark.dbCountOfBuggy != -1:
                idb.add(benchmark.name)
            if mapleRes[idx] == 1:
                maple.add(benchmark.name)
            if randr.numBuggy != 0:
                rand.add(benchmark.name)
            
            if benchmark.dbCountOfBuggy == -1 and \
               mapleRes[idx] != 1 and \
               randr.numBuggy == 0:
                none.add(benchmark.name)
    
    # Compute the union of all elements
    union = idb.union(maple).union(rand)
    
    # For each element compute its 'indicator'
    # (e.g. an indicator of 110 means element belongs to set1 and set2 but not set3)
    indicators = ['%d%d%d' % (a in idb, a in rand, a in maple) for a in union]
    
    # Use the standard Counter object (Python 2.7+) to count the frequency for each indicator
    from collections import Counter
    subsets = Counter(indicators)
    
    print subsets
    
    plt.figure(figsize=(4,4))
    # Provide the resulting dictionary as the subsets parameter to venn3:
    v = venn3(subsets, set_labels=('IDB', 'Rand', 'MapleAlg'), set_colors=('r', 'g', '#2C58E8'), alpha=0.4, ax=None)
    plt.text(0.7, -0.2, str(len(none)), va='center', ha='center')
    plt.savefig(filename, transparent=True, bbox_inches='tight')


def venn_diagram4(filename):
    
    
    #dfs = set()
    idb = set()
    rand = set()
    
    none = set()
    
    for idx, (sn,benchmark) in enumerate(sortedBenchmarks):
            assert isinstance(benchmark, Benchmark)
            dfsr = benchmark.dfsRes[0]
            dbr = benchmark.getDbBuggyRes()
            randr = benchmark.randomRes[0]
            assert isinstance(dfsr, BenchmarkSearchResults)
            assert isinstance(dbr, BenchmarkSearchResults)
            assert isinstance(randr, BenchmarkSearchResults)
            
            if dfsr.numSchedules < notFound or randr.numBuggy >= notFound:
                continue
            
            if benchmark.dbCountOfBuggy != -1:
                idb.add(benchmark.name)
            
            #if dfsr.numBuggy != 0:
            #    dfs.add(benchmark.name)
            
            if randr.numBuggy != 0:
                rand.add(benchmark.name)
            
            if benchmark.dbCountOfBuggy == -1 and \
               randr.numBuggy == 0:
                none.add(benchmark.name)
    
    # Compute the union of all elements
    union = idb.union(rand)
    
    # For each element compute its 'indicator'
    # (e.g. an indicator of 110 means element belongs to set1 and set2 but not set3)
    indicators = ['%d%d' % (a in idb, a in rand) for a in union]
    
    # Use the standard Counter object (Python 2.7+) to count the frequency for each indicator
    from collections import Counter
    subsets = Counter(indicators)
    
    print subsets
    
    plt.figure(figsize=(4,4))
    # Provide the resulting dictionary as the subsets parameter to venn3:
    v = venn2(subsets, set_labels=('IDB', 'Rand'), set_colors=('r', 'g', '#2C58E8'), alpha=0.4, ax=None)
    plt.text(0.7, -0.2, str(len(none)), va='center', ha='center')
    plt.savefig(filename, transparent=True, bbox_inches='tight')



try:
    with open("benchmarks.bin", 'rb') as fh:
        benchmarks = pickle.load(fh)
except IOError as e:
    if e.errno == 2:
        createBenchmarkBin()

modifiedNamesBenchmarks = []

def modifyBenchmarkNames():

    def escape(m):
        return "\\" + m.group(1)

    for (oldname, benchmark) in benchmarks.items():
        suite, name = oldname.split("--")
        if suite == "chess":
            name = re.sub('[a-z]', '', name)
        elif name == "ctrace-test":
            suite = "misc"
        elif suite == "safestack":
            suite = "misc"
            name = "safestack"
        elif suite == "conc-bugs":
            suite = "CB"
        elif suite == "concurrent-software-benchmarks":
            suite = "CS"
        elif suite.startswith("inspect"):
            suite = "inspect"
        elif suite.startswith("parsec"):
            suite = "parsec"


        newName = suite+"."+name
        benchmark.name = newName
        benchmark.latex_name = re.sub("([_])", escape, newName)
        modifiedNamesBenchmarks.append((newName, benchmark))
    
modifyBenchmarkNames()

sortedBenchmarks = sorted(modifiedNamesBenchmarks)
':type: list[tuple[str,Benchmark]]'



def printBuggy():
    print "Missed by all:"
    count=0
    for name,benchmark in sortedBenchmarks:
        assert isinstance(benchmark, Benchmark)
        #print str(count) + ": " + benchmark.name
        dfsRes = benchmark.dfsRes[0]
        pctRes = benchmark.pctRes[2]
        assert isinstance(pctRes, BenchmarkSearchResults)
        assert isinstance(dfsRes, BenchmarkSearchResults)
        if benchmark.pbCountOfBuggy == -1 and benchmark.dbCountOfBuggy == -1 and \
            dfsRes.numBuggy == 0 and benchmark.pctRes[1].numBuggy == 0 and \
                benchmark.pctRes[2].numBuggy == 0 \
                        and benchmark.pctRes[3].numBuggy == 0 \
                        and (not benchmark.mapleFound):
            print benchmark.name


        #print pctRes.maxThreads
        #print pctRes.maxSteps
        #print pctRes.numBuggy
        #print pctRes.numBeforeBuggy

        #if benchmark.pctRes.get(2) != -1 or benchmark.pbCountOfBuggy != -1:
        #    print str(count) + ": *" + benchmark.name
        #else:
        #    print str(count) + ": " + benchmark.name
        #count+=1


def writeTable2(filename):

    def writeFirst(s):
            fh.write(str(s))

    def write(s):
            if isinstance(s, int) and s == notFound:
                fh.write(sep + "L")
            else:
                fh.write(sep + str(s))
    def escape(m):
            return "\\" + m.group(1)

    sep = " & "
    nl = "\\\\\n"
    with open(filename, 'w') as fh:
        for idx, (name,benchmark) in enumerate(sortedBenchmarks):

            # benchmark id
            writeFirst(idx)

            # benchmark name
            temp = benchmark.name
            temp = re.sub("([_])", escape, temp)
            write(temp)

            write(benchmark.maxThreadsInput)
            write(benchmark.maxEnabledThreads)
            write(benchmark.maxSteps)

            # Random results
            toBuggy = benchmark.randomRes[0].numBeforeBuggy
            write("\\xmark{}" if toBuggy == -1 else toBuggy)
            write(benchmark.randomRes[0].numBuggy)


            # PCT: for each value of d
            for i in range(1, 4):
                if i not in benchmark.pctRes:
                    print "skipping pct d="+str(i)+" for "+benchmark.name
                    continue
                pct_res = benchmark.pctRes[i]
                assert isinstance(pct_res, BenchmarkSearchResults)

                est = benchmark.maxThreadsInput * pow(benchmark.maxSteps, i+1)
                estBuggy = float(benchmark.schedLimit) / float(est)

                #write("\\resizebox{!}{1ex}{$\\frac{1}{"+str(est)+"}$}")


                if pct_res.numBeforeBuggy == -1:
                    write("\\xmark{}")
                else:
                    write(pct_res.numBeforeBuggy)
                write(pct_res.numBuggy)

                if estBuggy < 1.0:
                    write("$<$1")
                else:
                    write(int(estBuggy))

            # MapleAlg
            write('\\cmark{}'if benchmark.mapleFound else '\\xmark{}')
            write(benchmark.mapleScheds)
            write(benchmark.mapleTime)

            fh.write(nl)


def writeCsv(filename, sep=",", nl="\n", writeHeadings=True, latex=False):
    with open(filename, 'w') as fh:
        assert isinstance(fh, file)
        def write(s):
            if isinstance(s, int) and s == notFound:
                fh.write(sep + "L")
            else:
                fh.write(sep + str(s))
        def write2(s):
            fh.write(sep + str(s))
        
        def writeFirst(s):
            fh.write(str(s))
        
        if writeHeadings:
            writeFirst("id")
            write("name")
            
            write("#threads")
            write("#max enabled threads")
            write("#scheduling points")
            
            write("bound")
            write("to first bug")
            write("#total")
            write("#total new")
            write("#total buggy")
            
            write("bound")
            write("to first bug")
            write("#total")
            write("#total new")
            write("#total buggy")
            
            write("to first bug")
            write("#total")
            write("#buggy")
            
            write("to first bug")
            write("#total")
            write("#buggy")
        
        fh.write("\n")
        def writeBuggyRes(res):
            assert isinstance(res, BenchmarkSearchResults)
            write(res.iterativeNumBeforeBuggy)
            write(res.numSchedules)
            write(res.numNewSchedules)
            write(res.iterativeNumBuggy)
        
        def escape(m):
            return "\\" + m.group(1)
        
        for idx, (name,benchmark) in enumerate(sortedBenchmarks):
            assert isinstance(benchmark, Benchmark)
            emptyRes = BenchmarkSearchResults()
            emptyRes.makeEmptyString()
            pbRes = emptyRes if benchmark.pbCountOfBuggy == -1 else benchmark.pbRes[benchmark.pbCountOfBuggy]
            dbRes = emptyRes if benchmark.dbCountOfBuggy == -1 else benchmark.dbRes[benchmark.dbCountOfBuggy]
            assert isinstance(pbRes, BenchmarkSearchResults)
            assert isinstance(dbRes, BenchmarkSearchResults)
            
            writeFirst(idx)
            
            if latex:
                temp = benchmark.name
                temp = re.sub("([_])", escape, temp)
                write(temp)
            else:
                write(benchmark.name)
            
            write(benchmark.maxThreadsInput)
            write(benchmark.maxEnabledThreads)
            #write(benchmark.dbRes[1].numSchedules)
            write(benchmark.maxSteps)

            print benchmark.name
            if benchmark.pbCountOfBuggy == -1:
                write(benchmark.pbBoundReached)
                write("\\xmark{}")
                write(benchmark.pbRes[benchmark.pbBoundReached].iterativeNumSchedules)
                write(benchmark.pbRes[benchmark.pbBoundReached].numNewSchedules)
                write(0)
            else:
                write(benchmark.pbCountOfBuggy)
                writeBuggyRes(pbRes)
            
            if benchmark.dbCountOfBuggy == -1:
                write(benchmark.dbBoundReached)
                write("\\xmark{}")
                write(benchmark.dbRes[benchmark.dbBoundReached].iterativeNumSchedules)
                write(benchmark.dbRes[benchmark.dbBoundReached].numNewSchedules)
                write(0)
            else:
                write(benchmark.dbCountOfBuggy)
                writeBuggyRes(dbRes)
            
            toBuggy = benchmark.dfsRes[0].numBeforeBuggy
            write("\\xmark{}" if toBuggy == -1 else toBuggy)
            totalSched = benchmark.dfsRes[0].numSchedules
            write(totalSched)
            write(benchmark.dfsRes[0].numBuggy)
            percBuggy = float(benchmark.dfsRes[0].numBuggy) / float(totalSched) * 100.0
            percBuggyInt = int(percBuggy)
            #write(str(percBuggy) + "\\%")
            percBuggyStr = str(percBuggyInt)
            
            if benchmark.dfsRes[0].numBuggy != 0 and percBuggy < 1.0:
                percBuggyStr = "$<$1"
            
#             if percBuggyStr.startswith("0.") and not percBuggyStr.endswith(".00"):
#                 percBuggyStr = "$<$0"
#             else:
#                 percBuggyStr = "{0:.0f}".format(percBuggy)
            
#             if percBuggyStr.endswith(".00"):
#                  percBuggyStr = percBuggyStr.replace(".00", "")
            if totalSched == benchmark.schedLimit:
                percBuggyStr = "*"+percBuggyStr
            write(percBuggyStr + "\\%")

            # Random results
            #toBuggy = benchmark.randomRes[0].numBeforeBuggy
            #write("\\xmark{}" if toBuggy == -1 else toBuggy)
            #write(benchmark.randomRes[0].numBuggy)
            
            
            fh.write(nl)
            
            


wordsDict = {0: 'Zero',
             1: 'One', 2: 'Two', 3: 'Three', 4: 'Four', 5: 'Five', 6: 'Six', 7: 'Seven',
                          8: 'Eight', 9: 'Nine'}

def writeMacros(filename):
    with open(filename, 'w') as fh:
        def write(m,v):
            fh.write("\\newcommand{\\" + str(m) + "}{" + str(v) + "}\n")
        def num2word(m):
            return wordsDict[int(m.group(1))]
        assert isinstance(fh, file)
        for idx, (sn,benchmark) in enumerate(sortedBenchmarks):
            assert isinstance(benchmark, Benchmark)
            m = re.match("(.*?)[.](.*)", sn)
            name = m.group(2)
            name = re.sub("[.\-_]", "", name)
            name = re.sub("([0-9])", num2word, name)
            # \newcommand{\numBuggyBenchmarks}{53}
            write(name, idx)
        
        justPb = 0
        justDb = 0
        both = 0
        neither = 0
        numBenchmarks = 0
        
        numFirstBugSimilar = 0
        numFirstBugDifferent = 0
        
        numWorstCaseSimilar = 0
        numWorstCaseDifferent = 0
        
        similar = 100
        
        numPbBetter = 0
        pbBetterMaxDifference = 0
        
        bothLessThanHundred = 0
        
        dfsLessThanHundred = 0
        
        numFoundBySchedBounding = 0
        numFoundByDFS = 0
        numFoundBySchedBoundingButNotDFS = 0
        
        numFoundByDB0 = 0
        numExhaustive = 0
        numBuggyRandOverHalf = 0
        numRandAllBuggy = 0

        numMapleMissed = 0
        numMapleFound = 0
        numMapleMissedFoundByOthers = 0

        numMissedByAll = 0
        
        for idx, (sn,benchmark) in enumerate(sortedBenchmarks):
            assert isinstance(benchmark, Benchmark)
            
            
            pbr = benchmark.getPbBuggyRes()
            dbr = benchmark.getDbBuggyRes()
            print sn
            dfsr = benchmark.dfsRes[0]
            
            randr = benchmark.randomRes[0]

            pctr = benchmark.pctRes
            
            assert isinstance(pbr, BenchmarkSearchResults)
            assert isinstance(dbr, BenchmarkSearchResults)
            assert isinstance(dfsr, BenchmarkSearchResults)
            assert isinstance(randr, BenchmarkSearchResults)

            fnd = False
            if pbr.numBuggy > 0 or dbr.numBuggy > 0 or dfsr.numBuggy > 0 or randr.numBuggy > 0:
                fnd = True
            for (i,r) in pctr.iteritems():
                assert isinstance(r, BenchmarkSearchResults)
                if r.numBuggy > 0:
                    fnd = True
                    break

            if benchmark.mapleFound:
                numMapleFound+=1
            else:
                numMapleMissed+=1
                if fnd:
                    numMapleMissedFoundByOthers+=1


            if not fnd:
                numMissedByAll += 1

            if randr.numBuggy > (benchmark.schedLimit/2):
                numBuggyRandOverHalf+=1
            
            if randr.numBuggy >= notFound:
                numRandAllBuggy+=1
            
            if dfsr.numSchedules != benchmark.schedLimit:
                numExhaustive+=1
            
            if benchmark.dbCountOfBuggy == 0:
                numFoundByDB0+=1
            
            differenceFirstBug = abs(pbr.iterativeNumBeforeBuggy - dbr.iterativeNumBeforeBuggy)
            
            pbWorstCase = (pbr.iterativeNumSchedules-pbr.iterativeNumBuggy)
            dbWorstCase = (dbr.iterativeNumSchedules-dbr.iterativeNumBuggy)
            differenceWorstCase = abs(pbWorstCase - dbWorstCase)
            
            if differenceFirstBug >= similar:
                numFirstBugDifferent += 1
            else:
                numFirstBugSimilar +=1
            
            if differenceWorstCase >= similar:
                numWorstCaseDifferent +=1
            else:
                numWorstCaseSimilar += 1
            
            if pbr.iterativeNumBeforeBuggy <= 100 and dbr.iterativeNumBeforeBuggy <= 100:
                bothLessThanHundred +=1
            
            if dfsr.numBuggy !=0 and dfsr.numBeforeBuggy < 100:
                dfsLessThanHundred+=1
            
            if pbr.iterativeNumBeforeBuggy < dbr.iterativeNumBeforeBuggy:
                pbBetterMaxDifference = max(pbBetterMaxDifference, 
                                            dbr.iterativeNumBeforeBuggy-
                                            pbr.iterativeNumBeforeBuggy)
                numPbBetter+=1
            
            if benchmark.dbCountOfBuggy == -1 and benchmark.pbCountOfBuggy == -1:
                neither+=1
            elif benchmark.dbCountOfBuggy != -1 and benchmark.pbCountOfBuggy != -1:
                both+=1
            elif benchmark.dbCountOfBuggy != -1 and benchmark.pbCountOfBuggy == -1:
                justDb+=1
            elif benchmark.dbCountOfBuggy == -1 and benchmark.pbCountOfBuggy != -1:
                justPb+=1
            else:
                assert False
            
            if benchmark.dbCountOfBuggy != -1 or benchmark.pbCountOfBuggy != -1:
                numFoundBySchedBounding+=1
                if dfsr.numBuggy == 0:
                    numFoundBySchedBoundingButNotDFS+=1
            
            if dfsr.numBuggy != 0:
                numFoundByDFS+=1
            
            numBenchmarks+=1

        write("numMissedByAll", numMissedByAll)

        write("numFoundJustPb", justPb)
        write("numFoundJustDb", justDb)
        
        write("numFoundDb", justDb+both)

        write("numMapleFound", numMapleFound)
        write("numMapleMissed", numMapleMissed)
        write("numMapleMissedFoundByOthers", numMapleMissedFoundByOthers)


        write("numFoundBoth", both)
        write("numFoundNeither", neither)
        assert numBenchmarks == justPb+justDb+both+neither
        
        write("numFoundBySchedBounding", numFoundBySchedBounding)
        write("numFoundByDFS", numFoundByDFS)
        write("numFoundBySchedBoundingButNotDFS", numFoundBySchedBoundingButNotDFS)
        
        write("numBenchmarks", numBenchmarks)
        write("numBugsFound", justDb+justPb+both)
        write("numBugsMissed", neither)
        
        write("numFirstBugSimilar",numFirstBugSimilar)
        write("numFirstBugDifferent",numFirstBugDifferent)
        
        write("numWorstCaseSimilar",numWorstCaseSimilar)
        write("numWorstCaseDifferent",numWorstCaseDifferent)
        
        write("bothLessThanHundred",bothLessThanHundred)
        write("dfsLessThanHundred",dfsLessThanHundred)
        
        write("numPbBetter",numPbBetter)
        write("pbBetterMaxDifference",pbBetterMaxDifference)
        
        write("numFoundByDBzero", numFoundByDB0)
        write("numExhaustive", numExhaustive)
        write("numBuggyRandOverHalf", numBuggyRandOverHalf)
        write("numRandAllBuggy", numRandAllBuggy)

graph1("graphFirstBugThenTotal.pdf")
graph2("graphNonBuggyThenTotal.pdf")
graph3("graphAll.pdf")
graph4("graphDfsIpb.pdf")
graph5("graphDfsIdb.pdf")
graphIdbVsDfs("graphIdbVsDfs.pdf")
graphWorstIdbVsWorstRandom("graphWorstIdbVsWorstRandom.pdf")

calc_cumulative()

graph_cumulative("graph_cumulative.pdf", benchmarks)
graph_cumulative2("graph_cumulative2.pdf", benchmarks)
graph_cumulative3("graph_cumulative3.pdf", benchmarks)

barchart("barChart.pdf", 0)
barchart("barChart2.pdf", 1)

venn_diagram1("vennDiagram.pdf")
venn_diagram2("vennDiagram2.pdf")
venn_diagram3("vennDiagram3.pdf")
venn_diagram4("vennDiagram4.pdf")

venn_diagram_random("vennDiagramRandom.pdf")
venn_diagram_pct_rand_idb("venn_diagram_pct_rand_idb.pdf")
venn_diagram_pct_rand_ipb("venn_diagram_pct_rand_ipb.pdf")
venn_diagram_pct_maple_idb("venn_diagram_pct_maple_idb.pdf")



writeTable2("tableRand.tex")

writeCsv("table.csv")
writeCsv("table.tex", " & ", "\\\\\n", False, latex=True)
writeMacros("genmacros.tex")

# paperDir = "/data/eclipseLatex/workspace/paper2014/"
# figuresDir = paperDir+"figures/" 

# # shutil.copy2("genmacros.tex", paperDir)
# # shutil.copy2("table.tex", paperDir)
# # shutil.copy2("graphFirstBugThenTotal.pdf", figuresDir)
# # shutil.copy2("graphNonBuggyThenTotal.pdf", figuresDir)
# # #shutil.copy2("graphAll.pdf", figuresDir)
# # #shutil.copy2("graphDfsIpb.pdf", figuresDir)
# # shutil.copy2("vennDiagram.pdf", figuresDir)
# # shutil.copy2("vennDiagramRandom.pdf", figuresDir)
# # shutil.copy2("venn_diagram_pct_rand_idb.pdf", figuresDir)
# # shutil.copy2("venn_diagram_pct_maple_idb.pdf", figuresDir)
# #  
# # shutil.copy2("graph_cumulative.pdf", figuresDir)
# # shutil.copy2("graph_cumulative2.pdf", figuresDir)
# # shutil.copy2("graph_cumulative3.pdf", figuresDir)
# # 
# #  
# # shutil.copy2("barChart.pdf", figuresDir)
# #  
# # shutil.copy2("tableRand.tex", paperDir)

# printBuggy()
