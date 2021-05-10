#! /usr/bin/env python3

import argparse
import os
import sys
import subprocess

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitMergeRun.py", "Submitter for runwise merging on the 587 cluster")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    parser.add_argument("-f", "--filename", metavar="FILENAME", default="AnalysisResults.root", type=str, help="File to be merged")
    parser.add_argument("-p", "--partition", metavar="PARTITION", default="short", type=str, help="Partition in slurm")
    args = parser.parse_args()
    inputdir = os.path.abspath(args.inputdir)
    repo = os.path.abspath(os.path.dirname(sys.argv[0]))
    outputbase = os.path.join(inputdir, "merged")
    if not os.path.exists(outputbase):
        os.makedirs(outputbase, 0o755)
    executable = os.path.join(repo, "processMergeMCDatasets.sh")
    exefinal = os.path.join(repo, "processMergeFinal.sh")
    resultSlots = subprocess.run("sbatch --array=1-20 --partition={PARTITION} -o {OUTPUTDIR}/joboutput_%a.log {EXECUTABLE} {INPUTDIR} {OUTPUTDIR} {FILENAME}".format(PARTITION=args.partition, OUTPUTDIR=outputbase, EXECUTABLE=executable, INPUTDIR=inputdir, FILENAME=args.filename), shell=True, stdout=subprocess.PIPE)
    soutSlots = resultSlots.stdout.decode("utf-8")
    toks = soutSlots.split(" ")
    jobid = int(toks[len(toks)-1])
    print("Submitted merge job under JobID %d" %jobid)
    resultFinal = subprocess.run("sbatch -N 1 -n1 --partition={PARTITION} -d {DEP} -J mergefinal -o {OUTPUTDIR}/mergefinal.log {EXEFINAL} {OUTPUTDIR} {FILENAME}".format(PARTITION=args.partition, DEP=jobid, OUTPUTDIR=outputbase, EXEFINAL=exefinal, FILENAME=args.filename), shell=True, stdout=subprocess.PIPE)
    soutFinal = resultFinal.stdout.decode("utf-8")
    toks = soutFinal.split(" ")
    jobidFinal = int(toks[len(toks)-1])
    print("Submitted final merging job under JobID %d" %jobidFinal)