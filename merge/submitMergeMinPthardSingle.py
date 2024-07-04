#! /usr/bin/env python3

import argparse
import os
import sys
import subprocess

def submitMergeMinpthard(sourcedir, workdir, minpthard, filename):
    executable = os.path.join(sourcedir, "processMergeMinPthardbin.sh")
    jobname = f"mergeMin_{minpthard}"
    logfile = os.path.join(workdir, f"jobmergemin_{minpthard}.log")
    cmd = f"{executable} {workdir} {minpthard} {filename}"
    sbatch = f"sbatch -N 1 -n 1 -J {jobname} -o {logfile}"
    subprocess.call(f"{sbatch} {cmd}"  , shell=True)

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitMergeMinPthardSingle.py", "Submitter for pt-hard dependent merge")
    parser.add_argument("minpthardbin", metavar="MINPTHARDBIN", type=int, help="Min. pt-hard bin")
    parser.add_argument("-w", "--workdir", metavar="WORKDIR", type=str, default=os.getcwd(), help="Working directory")
    parser.add_argument("-f", "--filename", metavar="FILENAME", type=str, default="AnalysisResults.root", help="File to merge")
    args = parser.parse_args()
    sourcedir = os.path.dirname(sys.argv[0])
    submitMergeMinpthard(sourcedir, args.workdir, args.minpthardbin, args.filename)