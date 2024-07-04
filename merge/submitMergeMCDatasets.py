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

def merge_submitter_datasets(repo: str, inputdir: str, filename: str, partition: str, maxtime: str, wait: list, check: bool, nofinal: bool = False) -> dict:
    outputbase = os.path.join(inputdir, "merged")
    if not os.path.exists(outputbase):
        os.makedirs(outputbase, 0o755)
    executable = os.path.join(repo, "processMergeMCDatasets.sh")
    exefinal = os.path.join(repo, "processMergeFinal.sh")
    runcmd_bins = f"{executable} {inputdir} {outputbase} {filename}"
    logfile_bins = os.path.join(outputbase, "joboutput_%a.log")
    jobid = submit_dependencies(runcmd_bins,
                                "mergebins", 
                                logfile_bins,
                                partition,
                                jobarray=[1,20],
                                dependency=wait,
                                maxtime=maxtime)
    logging.info("Submitted merge job under JobID %d", jobid)
    job_id_final = -1
    if not nofinal:
        finalcmd = f"{exefinal} {outputbase} {filename} {os.path.dirname(repo)} {1 if check else 0}"
        job_id_final = submit(finalcmd,
                              "mergefinal", 
                              f"{outputbase}/mergefinal.log", 
                              partition,
                              dependency=jobid,
                              maxtime=maxtime)
        logging.info("Submitted final merging job under JobID %d", job_id_final)
    return  {"pthard": jobid, "final": job_id_final}
   

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitMergeRun.py", "Submitter for runwise merging on the 587 cluster")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    parser.add_argument("-f", "--filename", metavar="FILENAME", default="AnalysisResults.root", type=str, help="File to be merged")
    parser.add_argument("-p", "--partition", metavar="PARTITION", default="short", type=str, help="Partition in slurm")
    parser.add_argument("-n", "--nofinal", action="store_true", help="Do not perform final merge job (i.e. for tree output)")
    parser.add_argument("--maxtime", metavar="MAXTIME", type=str, default="01:00:00", help="Maximum time for download job")
    parser.add_argument("-w", "--wait", metavar="WAIT", type=str, default="", help="List of jobs to depend on (spearated by \",\")")
    parser.add_argument("-c", "--check", action="store_true", help="Check pt-hard distribution")
    parser.add_argument("-d", "--debug", action="store_true", help="Debug mode")
    args = parser.parse_args()
    setup_logging(args.debug)
    INPUTDIR = os.path.abspath(args.inputdir)
    REPO = os.path.abspath(os.path.dirname(sys.argv[0]))

    CLUSTER = ""
    try:
        CLUSTER = get_cluster()
    except SubmissionHostNotSupportedException as err:
        logging.error("Submission error: %s", err)
        sys.exit(1)
    logging.info("Submitting download on Cluster %s", CLUSTER)

    dependencies = []
    if len(args.wait):
        dependencies = parse_jobs(args.wait)

    try:
        merge_submitter_datasets(REPO,
                                 INPUTDIR,
                                 args.filename,
                                 args.partition,
                                 args.maxtime,
                                 dependencies,
                                 args.check,
                                 args.nofinal)
    except UnknownClusterException as err:
        logging.error("Submission error: %s", err)
        sys.exit(1)
    except PartitionException as err:
        logging.error("Submission error: %s", err)
        sys.exit(1)
    