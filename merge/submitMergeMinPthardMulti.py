#! /usr/bin/env python3

import argparse
import os
import sys
import subprocess

def submitMergeMinpthard(sourcedir, workdir, minpthard, filename):
    executable = os.path.join(sourcedir, "processMergeMinPthardbin.sh")
    jobname = "mergeMin_%d" %minpthard
    logfile = os.path.join(workdir, "jobmergemin_%d.log" %minpthard)
    subprocess.call("sbatch -N 1 -n 1 --partition=vip -J %s -o %s %s %s %d %s" %(jobname, logfile, executable, workdir, minpthard, filename), shell=True)

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitMergeMinPthardMulti.py", "Submitter for pt-hard dependent merge")
    parser.add_argument("-w", "--workdir", metavar="WORKDIR", type=str, default=os.getcwd(), help="Working directory")
    parser.add_argument("-f", "--filename", metavar="FILENAME", type=str, default="AnalysisResults.root", help="File to merge")
    parser.add_argument("-n", "--nsteps", metavar="NSTEPS", type=int, default=6, help="Number of steps (min. pt-hard bin)")
    args = parser.parse_args()
    sourcedir = os.path.dirname(sys.argv[0])
    for ipth in range(1, args.nsteps):
        submitMergeMinpthard(sourcedir, args.workdir, ipth, args.filename)