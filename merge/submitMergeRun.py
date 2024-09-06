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

    def submit(self, wait_jobid: int = 0, field: str = "all") -> int:
        jobid_pthard = self.submit_pthardbins(wait_jobid, field)
        jobid_final = self.submit_final(jobid_pthard)
        return {"pthard": jobid_pthard, "final": jobid_final}

    def submit_pthardbins(self, wait_jobid: int = 0, field: str="all") -> int:
        executable = os.path.join(self.__repo, "processMergeRun.sh" if field == "all" else "processMergeRunsFieldMC.sh")
        command = f"{executable} {self.__inputdir} {self.__outputdir} {self.__filename}"
        if field != "all":
            command += f" {field} {os.path.dirname(self.__repo)}"
        logfile = f"{self.__outputdir}/joboutput_%a.log"
        jobname = "mergebins"
        try:
            jobid = submit(command=command, jobname=jobname, logfile=logfile, partition=self.__partition, jobarray=[1,20], dependency=wait_jobid, maxtime=self.__maxtime)
            return jobid
        except UnknownClusterException as err:
            logging.error("Submission error: %s", err)
            return -1
        except PartitionException as err: 
            logging.error("Submission error: %s", err)
            return -1

    def submit_final(self, wait_jobid: int = 0) -> int:
        executable = os.path.join(self.__repo, "processMergeFinal.sh")
        command = f"{executable} {self.__outputdir} {self.__filename} {os.path.dirname(self.__repo)} {1 if self.__check else 0}"
        logfile = f"{self.__outputdir}/mergefinal.log"
        jobname = "mergefinal"
        try:
            jobid = submit(command=command, jobname=jobname, logfile=logfile, partition=self.__partition, dependency=wait_jobid, maxtime=self.__maxtime)
            return jobid
        except UnknownClusterException as err:
           logging.error("Submission error: %s", err)
           return -1
        except PartitionException as err: 
           logging.error("Submission error: %s", err)
           return -1

def merge_submitter_runs(repo: str, inputdir: str, filename: str, partition: str, maxtime: str, wait: int, check: bool, field: str = "all") -> dict:
    mergetag = "merged"
    if field != "all":
        mergetag += f"_{field}"
    outputbase = os.path.join(inputdir, mergetag)
    if not os.path.exists(outputbase):
        os.makedirs(outputbase, 0o755)

    handler = MergeHandler(repo, inputdir, outputbase, filename, partition, check)
    jobids = handler.submit(wait, field)
    print(f"Submitted merge job under JobID {jobids['pthard']}")
    print(f"Submitted final merging job under JobID {jobids['final']}")
    return jobids

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitMergeRun.py", "Submitter for runwise merging on the 587 cluster")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    parser.add_argument("-f", "--filename", metavar="FILENAME", default="AnalysisResults.root", type=str, help="File to be merged")
    parser.add_argument("-p", "--partition", metavar="PARTITION", default="short", type=str, help="SLURM partition")
    parser.add_argument("-w", "--wait", metavar="WAIT", type=int, default=0, help="Wait for batch job to finish")
    parser.add_argument("-c", "--check", action="store_true", help="Check pt-hard distribution")
    parser.add_argument("-b", "--bfield", metavar="BFIELD", type=str, default="all", help="B-field (2018 only)", choices=["all", "pos", "neg"])
    parser.add_argument("--maxtime", metavar="MAXTIME", type=str, default="10:00:00", help="Maximum time for download job")
    parser.add_argument("-d", "--debug", action="store_true", help="Debug mode")
    args = parser.parse_args()
    setup_logging(args.debug)

    CLUSTER = ""
    try:
        CLUSTER = get_cluster()
    except SubmissionHostNotSupportedException as e:
        logging.error("Submission error: %s", e)
        sys.exit(1)
    logging.info("Submitting download on cluster %s", CLUSTER)

    REPO = os.path.abspath(os.path.dirname(sys.argv[0]))
    INPUTDIR = os.path.abspath(args.inputdir)
    merge_submitter_runs(REPO, INPUTDIR, args.filename, args.partition, args.maxtime, args.wait, args.check, args.bfield)