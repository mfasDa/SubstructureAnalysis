#! /usr/bin/env python3

import argparse
import os
import subprocess
import sys

def main():
    repo = os.getenv("SUBSTRUCTURE_ROOT")
    if not repo:
        print("Substructure repo not defined")
        sys.exit(1)

    parser = argparse.ArgumentParser("submitMergeRunsFieldData.py")
    parser.add_argument("-i", "--inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    parser.add_argument("-f", "--filename", metavar="FILENAME", type=str, default="AnalysisResults.root", help="ROOT file to be merged (default: AnalysisResults.root)")
    parser.add_argument("-p", "--partition", metavar="PARTITION", type=str, default="short", help="Partiion (default: short)")
    args = parser.parse_args()

    inputdir = os.path.abspath(args.inputdir)
    logdir = os.path.join(inputdir, "merge_logs")
    if not os.path.exists(logdir):
        os.makedirs(logdir, 0o755)
    executable = os.path.join(repo, "merge", "processMergeRunsFieldData.sh")
    for field in ["pos", "neg", "all"]:
        logfile = os.path.join(logdir, f"merge_{field}.log")
        jobname = f"merge_{field}"
        sbatch = f"sbatch -N 1 -c 11 --partition {args.partition} -J {jobname} -o {logfile}"
        cmd = f"{executable} {repo} {inputdir} {args.filename} {field}"
        subprocess.call(f"{sbatch} {cmd}", shell=True)

if __name__ == "__main__":
    main()