#! /usr/bin/env python3

import argparse
import logging
import os
import subprocess
import sys

substructure_repo = os.path.dirname(os.path.dirname(os.path.abspath(sys.argv[0])))

def submit_sort(inputdir: str, outputdir: str, rootfile: str, partition: str, runwise: bool):
    executable = os.path.join(substructure_repo, "downloader", "runsort.sh")
    sorttype = "runwise" if runwise else "periodwise"
    runcmd = "{EXECUTABLE} {REPO} {INPUTDIR} {OUTPUTDIR} {ROOTFILE} {RUNWISE}".format(EXECUTABLE=executable, REPO=substructure_repo, INPUTDIR=inputdir, OUTPUTDIR=outputdir, ROOTFILE=rootfile, RUNWISE=1 if runwise else 0)
    jobname = "sort_{}".format(sorttype)
    logfile = os.path.join(outputdir, "sort.log")
    submitcmd = "sbatch -N1 -n1 --partition {PARTITION} -J {JOBNAME} -o {LOGFILE} {RUNCMD}".format(PARTITION=partition, JOBNAME=jobname, LOGFILE=logfile, RUNCMD=runcmd)
    subprocess.call(submitcmd, shell=True)

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitSort.py", description="Submitter for pt-hard production sorter")
    parser.add_argument("-i", "--inputdir", metavar="INPUTDIR", type=str, required=True, help="Input directory") 
    parser.add_argument("-o", "--outputdir", metavar="OUTPUTDIR", type=str, required=True, help="Output directory")
    parser.add_argument("-f", "--file", metavar="ROOTFILE", type=str, default="AnalysisResults.root", help="Rootfile to sort (default: AnalysisResults.root)")
    parser.add_argument("-p", "--partition", metavar="PARTITION", type=str, default="short", help="Slurm partiton on the 587 cluster (default: short)")
    parser.add_argument("-r", "--runwise", action="store_true", help="Perform runwise sorting instead of periodwise sorting")
    parser.add_argument("-d", "--debug", action="store_true", help="Run in debug mode")
    args = parser.parse_args()
    
    loglevel = logging.INFO
    if args.debug:
        loglevel = logging.DEBUG
    logging.basicConfig(format="%(levelname)s: %(message)s", level=loglevel)

    if not os.path.exists(args.inputdir):
        logging.error("Input directory %s not existing. Exiting ...", args.inputdir)
        sys.exit(1)

    if not os.path.exists(args.outputdir):
        logging.info("Creating output directory %s ...", args.outputdir)
        os.makedirs(args.outputdir, 0o755)

    submit_sort(args.inputdir, args.outputdir, args.file, args.partition, args.runwise)