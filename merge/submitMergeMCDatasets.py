#! /usr/bin/env python3

import argparse
import os
import sys
import subprocess

def get_dependency_string(wait:list, mode: str) -> str:
    depstring = ""
    first = True
    for jobid in wait:
        if first:
            first = False
        else:
            depstring += ":"
        depstring += "{}".format(jobid)
    result = ""
    if len(depstring):
        result = "{MODE}:{JOBS}".format(MODE=mode, JOBS=depstring)
    return result

def parse_jobs(jobs: str) -> list:
    result = []
    if len(jobs):
        result = jobs.split(",")
    return result

def merge_submitter_datasets(repo: str, inputdir: str, filename: str, partition: str, wait: list, check: bool) -> dict:
    outputbase = os.path.join(inputdir, "merged")
    if not os.path.exists(outputbase):
        os.makedirs(outputbase, 0o755)
    executable = os.path.join(repo, "processMergeMCDatasets.sh")
    exefinal = os.path.join(repo, "processMergeFinal.sh")
    runcmd_bins = "{EXECUTABLE} {INPUTDIR} {OUTPUTDIR} {FILENAME}".format(EXECUTABLE=executable, INPUTDIR=inputdir, FILENAME=filename, OUTPUTDIR=outputbase)
    logfile_bins = os.path.join(outputbase, "joboutput_%a.log")
    slurm_bins = "sbatch --array=1-20 --partition={PARTITION} -o {LOGFILE}".format(PARTITION=partition, LOGFILE=logfile_bins)
    dependency_bins= get_dependency_string(wait, "afterany")
    if len(dependency_bins):
        slurm_bins += " --dependency={DEPSTRING}".format(DEPSTRING=dependency_bins)
    slurm_bins += " {}".format(runcmd_bins)
    resultSlots = subprocess.run(slurm_bins, shell=True, stdout=subprocess.PIPE)
    soutSlots = resultSlots.stdout.decode("utf-8")
    toks = soutSlots.split(" ")
    jobid = int(toks[len(toks)-1])
    print("Submitted merge job under JobID %d" %jobid)
    finalcmd = "{EXECUTABLE} {OUTPUTDIR} {FILENAME} {REPO} {CHECK}".format(EXECUTABLE=exefinal, OUTPUTDIR=outputbase, FILENAME=filename, REPO=repo, CHECK=1 if check else 0)
    resultFinal = subprocess.run("sbatch -N 1 -n1 --partition={PARTITION} -d {DEP} -J mergefinal -o {OUTPUTDIR}/mergefinal.log {MERGECMD}".format(PARTITION=partition, DEP=jobid, OUTPUTDIR=outputbase, MERGECMD=finalcmd), shell=True, stdout=subprocess.PIPE)
    soutFinal = resultFinal.stdout.decode("utf-8")
    toks = soutFinal.split(" ")
    jobidFinal = int(toks[len(toks)-1])
    print("Submitted final merging job under JobID %d" %jobidFinal)
    return  {"pthard": jobid, "final": jobidFinal}
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitMergeRun.py", "Submitter for runwise merging on the 587 cluster")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    parser.add_argument("-f", "--filename", metavar="FILENAME", default="AnalysisResults.root", type=str, help="File to be merged")
    parser.add_argument("-p", "--partition", metavar="PARTITION", default="short", type=str, help="Partition in slurm")
    parser.add_argument("-w", "--wait", metavar="WAIT", type=str, default="", help="List of jobs to depend on (spearated by \",\")")
    parser.add_argument("-c", "--check", action="store_true", help="Check pt-hard distribution")
    args = parser.parse_args()
    inputdir = os.path.abspath(args.inputdir)
    repo = os.path.abspath(os.path.dirname(sys.argv[0]))

    dependencies = []
    if(len(args.wait)):
        dependencies = parse_jobs(args.wait)

    merge_submitter_datasets(repo, inputdir, args.filename, args.partition, dependencies, args.check)
    