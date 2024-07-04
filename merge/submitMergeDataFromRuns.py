#! /usr/bin/env python3

import argparse
import logging
import os
import sys

from SubstructureHelpers.slurm import submit, submit_dependencies
from SubstructureHelpers.cluster import get_fast_partition, get_cluster, UnknownClusterException
from SubstructureHelpers.setup_logging import setup_logging

repository = os.environ["SUBSTRUCTURE_ROOT"]

def submit_merge_runs(cluster: str, basedir: str, period: str) -> int:
    workdir = os.path.join(basedir, period)
    executable = os.path.join(repository, "merge", "mergeRunsData.sh")
    jobname = f"mergeRuns_{period}"
    resultdir = os.path.join(workdir, "merged")
    if not os.path.exists(resultdir):
        os.makedirs(resultdir, 0o755)
    logfile = os.path.join(resultdir, "merge.log")
    mergecmd = f"{executable} {workdir}"
    return submit(mergecmd, jobname, logfile, get_fast_partition(cluster))

def submit_merge_final(cluster: str, basedir: str, dependencies: list) -> int:
    executable = os.path.join(repository, "merge", "mergePeriodsData.sh")
    resultdir = os.path.join(basedir, "merged")
    if not os.path.exists(resultdir):
        os.makedirs(resultdir, 0o755)
    logfile = os.path.join(resultdir, "merge.log")
    jobname = "mergeFinal_Data"
    mergecmd = f"{executable} {basedir}"
    return submit_dependencies(mergecmd, jobname, logfile, get_fast_partition(cluster), dependency=dependencies)

def submit_periods(cluster: str, basedir: str):
    periods = sorted([x for x in os.listdir(basedir) if x.startswith("LHC") and os.path.isdir(os.path.join(basedir, x))])
    jobids = []
    for period in periods:
        jobid = submit_merge_runs(cluster, basedir, period)
        logging.info("Submitting run merge job for period %s: %d", period, jobid)
        jobids.append(jobid)
    finalmergejob = submit_merge_final(cluster, basedir, jobids)
    logging.info("Submitting final merge job under job ID %d", finalmergejob)

def main():
    parser = argparse.ArgumentParser("submitMergeDataFromRuns.py")
    parser.add_argument("basedir", metavar="BASEDIR", type=str, help="Working directory")
    parser.add_argument("-d", "--debug", action="store_true", help="Debug mode")
    args = parser.parse_args()

    setup_logging(args.debug)
    cluster = ""
    try:
        cluster = get_cluster()
    except UnknownClusterException as err:
        logging.error(err)
        sys.exit(1)
    
    submit_periods(cluster, os.path.abspath(args.basedir))

if __name__ == "__main__":
    main()