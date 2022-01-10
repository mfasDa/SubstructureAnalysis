#! /usr/bin/env python3

import argparse
import os
import sys
from SubstructureHelpers import slurm

repo = os.getenv("SUBSTRUCTURE_ROOT")

def submit_run(workdir: str, runnumber: int):
    logfile = os.path.join(workdir, "trendingExtractor.log")
    jobname = "TD_{RUN}".format(RUN=runnumber)
    executable = os.path.join(repo, "Trending", "runExtractTrendingTree.sh")
    cmd = "{EXE} {REPO} {WD} {RUN}".format(EXE=executable, REPO=repo, WD=workdir, RUN=runnumber)
    slurm.submit(cmd, jobname, logfile)

def find_runs(basedir: str):
    return sorted([int(x) for x in os.listdir(basedir) if x.isdigit()])

def find_periods(basedir: str):
    return sorted([x for x in os.listdir(basedir) if x.startswith("LHC")])

def submit_runs(basedir: str):
    for r in find_runs(basedir):
        workdir = os.path.join(basedir, "%09d" %r)
        print("Submitting run: %d" %r)
        submit_run(workdir, r)

def submit_all(basedir: str):
    for p in find_periods(basedir):
        workdir = os.path.join(basedir, p)
        submit_runs(workdir)

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitTrending.py", description="Submitter for trending extraction")
    parser.add_argument("mode", metavar="MODE", type=str, help="Run mode (full or runs)")
    parser.add_argument("-w", "--workdir", metavar="WORKDIR", type=str, default=os.getcwd(), help="Working directory")
    args = parser.parse_args()

    modes = ["full", "runs"]
    if not args.mode in modes:
        print("Unsupported mode: %s" %args.mode)
        sys.exit(1)
    if args.mode == "full":
        submit_all(os.path.abspath(args.workdir))
    else:
        submit_runs(os.path.abspath(args.workdir))
