#! /usr/bin/env python3

import argparse
import logging
import os
import sys
from SubstructureHelpers.cluster import get_cluster, SubmissionHostNotSupportedException
from SubstructureHelpers.slurm import submit
from SubstructureHelpers.setup_logging import setup_logging

if __name__ == "__main__":
    repo = os.path.dirname(os.path.abspath(sys.argv[0]))
    executable = os.path.join(repo, "processMergeFinal.sh")

    parser = argparse.ArgumentParser("submitMergeFinal.py", description="Submitter for final merge job")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Iput directory for merge")
    parser.add_argument("-r", "--rootfile", metavar="ROOTFILE", type=str, default="AnalysisResults.root", help="Name of the ROOT file")
    parser.add_argument("-p", "--partition", metavar="PARTITION", type=str, default="fast", help="Queue for running")
    parser.add_argument("-c", "--check", action="store_true", help="Check pt-hard distribution")
    parser.add_argument("-d", "--debug", action="store_true", help="Debug mode")
    parser.add_argument("--maxtime", metavar="MAXTIME", type=str, default="2:00:00", help="Max. time for job (default: 2 hours)")
    args = parser.parse_args()
    setup_logging(args.debug)

    cluster = ""
    try:
        cluster = get_cluster()
    except SubmissionHostNotSupportedException as e:
        logging.error("[Submission error] %s", e)
        sys.exit(1)
    logging.info("Submitting download on cluster %s", cluster)

    cmd = "{EXE} {INPUTDIR} {ROOTFILE} {REPO} {CHECK}".format(EXE=executable, INPUTDIR=args.inputdir, ROOTFILE=args.rootfile, REPO=repo, CHECK=1 if args.check else 0)
    logfile = os.path.join(args.inputdir, "merge_final.log")
    jobname = "merge_final"
    try:
        jobid = submit(cmd, "merge_final", logfile, args.partition, maxtime=args.maxtime)
        logging.info("Submitted final merging job with ID %d", jobid)
    except Exception as e:
        logging.error("[Submission error] %s", e)
