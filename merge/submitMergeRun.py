#! /usr/bin/env python3

import argparse
import logging
import os
import sys

from SubstructureHelpers.cluster import get_cluster, SubmissionHostNotSupportedException, UnknownClusterException, PartitionException
from SubstructureHelpers.setup_logging import setup_logging
from SubstructureHelpers.slurm import submit

class MergeHandler:

    def __init__(self, repo: str, inputdir: str, outputdir: str, filename: str, partition: str = "short", maxtime: str = "10:00:00", check: bool = False):
        self.__repo = repo
        self.__inputdir = inputdir
        self.__outputdir = outputdir
        self.__filename = filename
        self.__partition = partition
        self.__maxtime = maxtime
        self.__check = check

    def submit(self, wait_jobid: int = 0) -> int:
        jobid_pthard = self.submit_pthardbins(wait_jobid)
        jobid_final = self.submit_final(jobid_pthard)
        return {"pthard": jobid_pthard, "final": jobid_final}

    def submit_pthardbins(self, wait_jobid: int = 0) -> int:
        executable = os.path.join(self.__repo, "processMergeRun.sh")
        commmand = "{EXECUTABLE} {INPUTDIR} {OUTPUTDIR} {FILENAME}".format(EXECUTABLE=executable, INPUTDIR=self.__inputdir, OUTPUTDIR=self.__outputdir, FILENAME=self.__filename)
        logfile = "{OUTPUTDIR}/joboutput_%a.log".format(OUTPUTDIR=self.__outputdir)
        jobname = "mergebins"
        try:
            jobid = submit(command=commmand, jobname=jobname, logfile=logfile, partition=self.__partition, jobarray=[1,20], dependency=wait_jobid, maxtime=self.__maxtime)
            return jobid
        except UnknownClusterException as e: 
           logging.error("Submission error: %s", e)
           return -1
        except PartitionException as e: 
           logging.error("Submission error: %s", e)
           return -1

    def submit_final(self, wait_jobid: int = 0) -> int:
        executable = os.path.join(self.__repo, "processMergeFinal.sh")
        command = "{EXECUTABLE} {OUTPUTDIR} {FILENAME} {REPO} {CHECK}".format(EXECUTABLE=executable, OUTPUTDIR=self.__outputdir, FILENAME=self.__filename, REPO=os.path.dirname(self.__repo), CHECK=1 if self.__check else 0)
        logfile = "{OUTPUTDIR}/mergefinal.log".format(OUTPUTDIR=self.__outputdir)
        jobname = "mergefinal"
        try:
            jobid = submit(command=command, jobname=jobname, logfile=logfile, partition=self.__partition, dependency=wait_jobid, maxtime=self.__maxtime)
            return jobid
        except UnknownClusterException as e: 
           logging.error("Submission error: %s", e)
           return -1
        except PartitionException as e: 
           logging.error("Submission error: %s", e)
           return -1

def merge_submitter_runs(repo: str, inputdir: str, filename: str, partition: str, maxtime: str, wait: int, check: bool) -> dict:
    outputbase = os.path.join(inputdir, "merged")
    if not os.path.exists(outputbase):
        os.makedirs(outputbase, 0o755)

    handler = MergeHandler(repo, inputdir, outputbase, filename, partition, check)
    jobids = handler.submit(wait)
    print("Submitted merge job under JobID %d" %jobids["pthard"])
    print("Submitted final merging job under JobID %d" %jobids["final"])
    return jobids

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitMergeRun.py", "Submitter for runwise merging on the 587 cluster")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    parser.add_argument("-f", "--filename", metavar="FILENAME", default="AnalysisResults.root", type=str, help="File to be merged")
    parser.add_argument("-p", "--partition", metavar="PARTITION", default="short", type=str, help="SLURM partition")
    parser.add_argument("-w", "--wait", metavar="WAIT", type=int, default=0, help="Wait for batch job to finish")
    parser.add_argument("-c", "--check", action="store_true", help="Check pt-hard distribution")
    parser.add_argument("--maxtime", metavar="MAXTIME", type=str, default="10:00:00", help="Maximum time for download job")
    parser.add_argument("-d", "--debug", action="store_true", help="Debug mode")
    args = parser.parse_args()
    setup_logging(args.debug)

    cluster = ""
    try:
        cluster = get_cluster()
    except SubmissionHostNotSupportedException as e:
        logging.error("Submission error: %s", e)
        sys.exit(1)
    logging.info("Submitting download on cluster %s", cluster)

    repo = os.path.abspath(os.path.dirname(sys.argv[0]))
    inputdir = os.path.abspath(args.inputdir)
    merge_submitter_runs(repo, inputdir, args.filename, args.partition, args.maxtime, args.wait, args.check)