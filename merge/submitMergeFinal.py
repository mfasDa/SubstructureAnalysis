#! /usr/bin/env python3

import argparse
import os
import subprocess
import sys

if __name__ == "__main__":
    repo = os.path.dirname(os.path.abspath(sys.argv[0]))
    executable = os.path.join(repo, "processMergeFinal.sh")

    parser = argparse.ArgumentParser("submitMergeFinal.py", description="Submitter for final merge job")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Iput directory for merge")
    parser.add_argument("-r", "--rootfile", metavar="ROOTFILE", type=str, default="AnalysisResults.root", help="Name of the ROOT file")
    parser.add_argument("-p", "--partition", metavar="PARTITION", type=str, default="vip", help="Queue for running")
    parser.add_argument("-c", "--check", action="store_true", help="Check pt-hard distribution")
    args = parser.parse_args()

    cmd = "{EXE} {INPUTDIR} {ROOTFILE} {REPO} {CHECK}".format(EXE=executable, INPUTDIR=args.inputdir, ROOTFILE=args.rootfile, REPO=repo, CHECK=1 if args.check else 0)
    logfile = os.path.join(args.inputdir, "merge_final.log")
    jobname = "merge_final"
    submitcmd = "sbatch -N1 -n1 --partition={PARTITION} -J {JOBNAME} -o {LOGFILE} {PROGRAM}".format(PARTITION=args.partition, JOBNAME=jobname, LOGFILE=logfile, PROGRAM=cmd)
    subprocess.call(submitcmd, shell=True)
