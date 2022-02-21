#! /usr/bin/env python3

import argparse
import logging
import os
import sys

from SubstructureHelpers.setup_logging import setup_logging
from SubstructureHelpers.cluster import get_cluster, SubmissionHostNotSupportedException, UnknownClusterException, PartitionException
from SubstructureHelpers.slurm import submit, submit_dependencies

def parse_jobs(jobs: str) -> list:
    result = []
    if len(jobs):
        result = jobs.split(",")
    return result

def merge_submitter_datasets(repo: str, inputdir: str, filename: str, partition: str, maxtime: str, wait: list, check: bool) -> dict:
    outputbase = os.path.join(inputdir, "merged")
    if not os.path.exists(outputbase):
        os.makedirs(outputbase, 0o755)
    executable = os.path.join(repo, "processMergeMCDatasets.sh")
    exefinal = os.path.join(repo, "processMergeFinal.sh")
    runcmd_bins = "{EXECUTABLE} {INPUTDIR} {OUTPUTDIR} {FILENAME}".format(EXECUTABLE=executable, INPUTDIR=inputdir, FILENAME=filename, OUTPUTDIR=outputbase)
    logfile_bins = os.path.join(outputbase, "joboutput_%a.log")
    jobid = submit_dependencies(runcmd_bins, "mergebins", logfile_bins, partition, jobarray=[1,20], dependency=wait, maxtime=maxtime)
    print("Submitted merge job under JobID %d" %jobid)
    finalcmd = "{EXECUTABLE} {OUTPUTDIR} {FILENAME} {REPO} {CHECK}".format(EXECUTABLE=exefinal, OUTPUTDIR=outputbase, FILENAME=filename, REPO=os.path.dirname(repo), CHECK=1 if check else 0)
    jobidFinal = submit(finalcmd, "mergefinal", "{OUTDIR}/mergefinal.log", partition, dependency=jobid, maxtime=maxtime)
    print("Submitted final merging job under JobID %d" %jobidFinal)
    return  {"pthard": jobid, "final": jobidFinal}
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitMergeRun.py", "Submitter for runwise merging on the 587 cluster")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    parser.add_argument("-f", "--filename", metavar="FILENAME", default="AnalysisResults.root", type=str, help="File to be merged")
    parser.add_argument("-p", "--partition", metavar="PARTITION", default="short", type=str, help="Partition in slurm")
    parser.add_argument("--maxtime", metavar="MAXTIME", type=str, default="01:00:00", help="Maximum time for download job")
    parser.add_argument("-w", "--wait", metavar="WAIT", type=str, default="", help="List of jobs to depend on (spearated by \",\")")
    parser.add_argument("-c", "--check", action="store_true", help="Check pt-hard distribution")
    parser.add_argument("-d", "--debug", action="store_true", help="Debug mode")
    args = parser.parse_args()
    setup_logging(args.debug)
    inputdir = os.path.abspath(args.inputdir)
    repo = os.path.abspath(os.path.dirname(sys.argv[0]))

    cluster = ""
    try:
        cluster = get_cluster()
    except SubmissionHostNotSupportedException as e:
        logging.error("Submission error: %s", e)
        sys.exit(1)
    logging.info("Submitting download on cluster %s", cluster)

    dependencies = []
    if(len(args.wait)):
        dependencies = parse_jobs(args.wait)

    try:
        merge_submitter_datasets(repo, inputdir, args.filename, args.partition, args.maxtime, dependencies, args.check)
    except UnknownClusterException as e:
        logging.error("Submission error: %s", e)
        sys.exit(1)
    except PartitionException as e:
        logging.error("Submission error: %s", e)
        sys.exit(1)
    