# SCTBench

A set of C/C++ pthread benchmarks and tools for evaluating concurrency testing techniques.

You can download our original VirtualBox artifact to reproduce our experiments [here](https://sites.google.com/site/sctbenchmarks/).
This git repository is supposed to be more convenient and includes more recent changes,
such as our implementation of the PCT algorithm.
Usability of the tools and scripts could still be greatly improved.
Nevertheless, it should be possible to reproduce our experimental evaluation.

Recall that the data race detection phase is nondeterministic
and different compiler versions may also lead to different binaries.
Thus, our study is not deterministic.
However, we provide a link to our log files so
additional analysis of our results is possible and
the graphs and tables can be reproduced deterministically. 

# Instructions

Without vagrant: (Requires kernel version <= 3.x due to PIN.)

```bash
sudo apt-get install ... (see vagrant_provision.sh)
cp env.template env.sh (edit env.sh as needed)
source env.sh
```

With vagrant:

```bash
vagrant up
vagrant ssh
cd /vagrant
source env.template
```

From the sourced shell:

```bash
build_maple
build_benchmarks
```

You must first run data race detection. From `$ROOT` (e.g. `/vagrant`).

```bash
run_many.py --mode race --max 10 benchmarks/buggy.txt
```

This will run data race detection 10 times on each benchmark, 
creating a database of data races for each benchmark.
E.g. `benchmarks/chess/obj/InterlockedWorkStealQueue/run_race`.
You should delete the `run_race` directories if you want to
re-run data race detection; otherwise, additional data race detection
runs will add to the existing set of data races for each benchmark.
A log file for each execution will be output. E.g.

* `benchmarks/__results/2016-11-22-10-45-22--chess--InterlockedWorkStealQueue--0race--0--.txt`
* `benchmarks/__results/2016-11-22-10-45-23--chess--InterlockedWorkStealQueue--0race--1--.txt`
* ...

The race detection log files are useful to see that the benchmarks
are running as expected. They will not be read by the
scripts that create the result tables and graphs.

You can now test the benchmarks using the different techniques:

```bash
run_many.py --mode dfs     --limit 100 --timelimit 300 benchmarks/buggy.txt
run_many.py --mode pb      --limit 100 --timelimit 300 benchmarks/buggy.txt
run_many.py --mode db      --limit 100 --timelimit 300 benchmarks/buggy.txt
run_many.py --mode random  --limit 100 --timelimit 300 benchmarks/buggy.txt
run_many.py --mode pct     --limit 100 --timelimit 300 benchmarks/buggy.txt
```

The above commands use a terminal schedule limit of 100 
and a time limit of 300 seconds (5 mins).
You can add `-bi 0` to just run the first benchmark to see
if things are working as expected.
We recommend a schedule limit of at least 10,000. 
Note that preemption bounding and delay bounding
first run with a bound of 0, then 1, etc.
This continues until a run hits the schedule limit or
all schedules are explored in a run. Thus,
the schedule limit may be exceeded across runs.
However, any extra schedules can be ignored by the scripts
that scan the log files. 

A log file will be output for each benchmark
(and, for `pb` and `db`, for each bound). E.g.

* `benchmarks/__results/2016-11-22-10-53-51/2016-11-22-10-53-51--chess--InterlockedWorkStealQueue--pb--0--.txt`   
* `benchmarks/__results/2016-11-22-10-53-51/2016-11-22-10-53-54--chess--InterlockedWorkStealQueue--pb--1--.txt`

The log files will be scanned by the scripts that generate the result tables and graphs.
You could also write your own scripts to scan these log files.

# Generating results tables and graphs

We used the script: `scripts/tabulate2.py`:

* You need `matplotlib` and possibly other Python modules.
* You also have to rename the results directories to include the word `use`.
* You have to have a complete set of results for ALL benchmarks.

In other words, the script is not easy to use in its current form. 
The results table and PDFs are output in the `benchmarks/` directory.

You can download our log files to reproduce our results:

```bash
cd benchmarks
mkdir __results
cd __results
wget https://www.dropbox.com/s/41k5e649m1c9lar/2015-06-100_000_after_reviews.tar.bz2?dl=1
tar -xvf 2015-06-100_000_after_reviews.tar.bz2
ls
# Should see the following:
# 2015-06-100_000_after_reviews.tar.bz2 use-2015-01-23-17-33-08--100_000_pb_db   use-2015-03-07-19-50-18--100_000_pct3
# use-2015-01-23-17-28-54--100_000_pct1  use-2015-01-23-17-34-38--100_000_random  use-2015-05-28-14-36-21--10_000_chess_pb_db
# use-2015-01-23-17-32-10--100_000_dfs   use-2015-03-07-19-49-58--100_000_pct2    use-2015-05-28-14-36-41--10_000_chess_dfs
cd ../..
tabluate2.py
ls benchmarks/
# Should see:
# barChart2.pdf                   graph_cumulative2.pdf    ... etc.
```

Once the log files have been scanned, the data is cached in `benchmarks/benchmarks.bin`
so you can regenerate graphs and tables without having to rescan logs files.

