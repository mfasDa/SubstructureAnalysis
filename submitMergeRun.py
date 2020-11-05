#! /usr/bin/env python

import argparse
import os
import sys
import subprocess

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitMergeRun.py", "Submitter for runwise merging on the 587 cluster")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    parser.add_argument("-f", "--filename", metavar="FILENAME", default="AnalysisResults.root", type=str, help="File to be merged")
    args = parser.parse_args()
    inputdir = os.path.abspath(args.inputdir)
    repo = os.path.abspath(os.path.dirname(sys.argv[0]))
    outputbase = os.path.join(inputdir, "merged")
    if not os.path.exists(outputbase):
        os.makedirs(outputbase, 0o755)
    executable = os.path.join(repo, "processMergeRun.sh")
    exefinal = os.path.join(repo, "processMergeFinal.sh")
    jobid = subprocess.call("sbatch --array=1-20 --partition=short -o {OUTPUTDIR}/joboutput_%a.log {EXECUTABLE} {INPUTDIR} {OUTPUTDIR} {FILENAME}".format(OUTPUTDIR=outputbase, EXECUTABLE=executable, INPUTDIR=inputdir, FILENAME=args.filename), shell=True)
    print("Submitted merge job under JobID %d" %jobid)
    jobfinale = subprocess.call("sbatch -N 1 -n1 -d {DEP} -J mergefinal -o {OUTPUTDIR}/mergefinal.log {EXEFINAL} {OUTPUTDIR} {FILENAME}".format(DEP=jobid, OUTPUTDIR=outputbase, EXEFINAL=exefinal, FILENAME=args.filename), shell=True)
    print("Submitted final merging job under JobID %d" %jobid)