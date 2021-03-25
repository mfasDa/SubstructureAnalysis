#! /usr/bin/env python3

import argparse
import os
import sys
import subprocess

def submit(command: str, jobname: str, logfile: str, partition: str = "short", numnodes: int = 1, numtasks: int = 1, jobarray = None, dependency=0) -> int:
    submitcmd = "sbatch -N {NUMNODES} -n ${NUMTASKS} --partition={PARTITION}".format(NUMNODES=numnodes, NUMTASKS=numtasks, PARTITION=partition)
    if jobarray:
        submitcmd += " --array={ARRAYMIN}-{ARRAYMAX}".format(ARRAYMIN=jobarray[0], ARRAYMAX=jobarray[1])
    if dependency > 0:
        submitcmd += " -d {DEP}".format(DEP=dependency)
    submitcmd += " -j {JOBNAME} -o {LOGFILE} {COMMAND}".format(JOBNAME=jobname, LOGFILE=logfile, COMMAND=command)
    submitResult = subprocess.run(submitcmd, shell=True, stdout=subprocess.PIPE)
    sout = submitResult.stdout.decode("utf-8")
    toks = sout.split(" ")
    jobid = int(toks[len(toks)-1])
    return jobid

class MergeHandler:

    def __init__(self, repo: str, inputdir: str, outputdir: str, filename: str, partition: str = "short"):
        self.__repo = repo
        self.__inputdir = inputdir
        self.__outputdir = outputdir
        self.__filename = filename
        self.__partition = partition

    def submit(self, wait_jobid: int = 0) -> int:
        jobid_pthard = self.submit_pthardbins(wait_jobid)
        jobid_final = self.submit_final(jobid_pthard)
        return {"pthard": jobid_pthard, "final": jobid_final}

    def submit_pthardbins(self, wait_jobid: int = 0) -> int:
        executable = os.path.join(self.__repo, "processMergeRun.sh")
        commmand = "{EXECUTABLE} {INPUTDIR} {OUTPUTDIR} {FILENAME}".format(EXECUTABLE=executable, INPUTDIR=self.__inputdir, OUTPUTDIR=self.__outputdir, FILENAME=self.__filename)
        logfile = "{OUTPUTDIR}/joboutput_%a.log"
        jobname = "mergebins"
        jobid = submit(command=commmand, jobname=jobname, logfile=logfile, partition=self.__partition, jobarray=[1,20], dependency=wait_jobid)
        return jobid

    def submit_final(self, wait_jobid: int = 0) -> int:
        executable = os.path.join(repo, "processMergeFinal.sh")
        command = "{EXECUTABLE} {OUTPUTDIR} {FILENAME}".format(EXECUTABLE=executable, OUTPUTDIR=self.__outputdir, FILENAME=self.__filename)
        logfile = "{OUTPUTDIR}/mergefinal.log".format(OUTPUTDIR=self.__outputdir)
        jobname = "mergefinal"
        jobid = submit(command=command, jobname=jobname, logfile=logfile, partition=self.__partition, dependency=wait_jobid)
        return jobid

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitMergeRun.py", "Submitter for runwise merging on the 587 cluster")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    parser.add_argument("-f", "--filename", metavar="FILENAME", default="AnalysisResults.root", type=str, help="File to be merged")
    parser.add_argument("-p", "--partition", metavar="PARTITION", default="short", type=str, help="SLURM partition")
    parser.add_argument("-w", "--wait", metavar="WAIT", type=int, default=0, help="Wait for batch job to finish")
    args = parser.parse_args()
    inputdir = os.path.abspath(args.inputdir)
    repo = os.path.abspath(os.path.dirname(sys.argv[0]))
    outputbase = os.path.join(inputdir, "merged")
    if not os.path.exists(outputbase):
        os.makedirs(outputbase, 0o755)

    handler = MergeHandler(repo, inputdir, outputbase, args.filename)
    jobids = handler.submit()
    print("Submitted merge job under JobID %d" %jobids["pthard"])
    print("Submitted final merging job under JobID %d" %jobids["final"])