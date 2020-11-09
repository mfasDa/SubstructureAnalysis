#! /usr/bin/env python3

import argparse
import os
import sys
import subprocess

def submitMergeMinpthard(sourcedir, workdir, minpthard, filename):
    executable = os.path.join(sourcedir, "processMergeMinpthard.sh")
    jobname = "mergeMin_%d" %minpthard
    logfile = os.path.join(workdir, "jobmergemin_%d.log" %minpthard)
    subprocess.call("sbatch -N 1 -n 1 -J %s -o %s %s %s %d %s" %(jobname, logfile, executable, workdir, minpthard, filename), shell=True)

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitMergeMinPthard.py", "Submitter for pt-hard dependent merge")
    parser.add_argument("-w", "--workdir", metavar="WORKDIR", type=str, default=os.getcwd(), help="Working directory")
    parser.add_argument("-f", "--filename", metavar="FILENAME", type=str, default="AnalysisResults.root", help="File to merge")
    args = parser.parse_args()
    sourcedir = os.path.dirname(sys.argv[0])
    for ipth in range(1, 6):
        submitMergeMinpthard(sourcedir, args.workdir, ipth, args.filename)