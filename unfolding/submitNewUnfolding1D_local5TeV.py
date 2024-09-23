#! /usr/bin/env python3

import argparse
import logging
import os

from SubstructureHelpers.setup_logging import setup_logging
from SubstructureHelpers import slurm

def main():
    repo = os.getenv("SUBSTRUCTURE_ROOT")
    parser = argparse.ArgumentParser("submitUnfolding1D_local", "Submitter for 1D unfolding")
    parser.add_argument("datafile", metavar="DATFILE2017", type=str, help="Data input from 2017")
    parser.add_argument("mcfile", metavar="MCFILE", type=str, help="MC input")
    parser.add_argument("-w", "--workdir", metavar="WORKDIR", type=str, default=os.getcwd(), help="Working directory")
    parser.add_argument("-s", "--sysvar", metavar="SYSVAR", type=str, default="default", help="Systematics variation")
    parser.add_argument("-m", "--macro", metavar="MACRO", type=str, default="runNewCorrectionChainMBonly1DSVD5TeV_SpectrumTaskSimplePoor_CorrectEffPure.cpp", help="Unfolding macro")
    parser.add_argument("-j", "--jobtag", metavar="JOBTAG", type=str, default="corr1D", help="Job tag in jobname")
    parser.add_argument("-q", "--queue", metavar="QUEUE", type=str, default="fast", help="Slurm queue")
    parser.add_argument("-t", "--time", metavar="MAXTIME", type=str, default="2:00:00", help="Max. time unfolding (for clusters which allocate time)")
    parser.add_argument("-r", "--radii", metavar="RADII", type=str, default="all", help="Comma-separated list of radii or all")
    parser.add_argument("-d", "--debug", action="store_true", help="Debug mode")
    args = parser.parse_args()

    setup_logging(args.debug)
    unfoldingexecutable = os.path.join(repo, "unfolding", "runNewUnfolding1D_local5TeV.sh")
    unfoldingcmd=f"{unfoldingexecutable} {repo} {args.workdir} {args.datafile} {args.mcfile} {args.sysvar} {args.macro}"
    logfile="joboutput_R0%a.log"
    workdir = os.path.abspath(args.workdir)
    if not os.path.exists(workdir):
        os.makedirs(workdir, 0o755)
    os.chdir(workdir)

    jobarray=[]
    if args.radii == "all":
        jobarray=[2,6]
    else:
        jobarray=["indices"]
        radii = args.radii.split(",")
        for rad in radii:
            if rad.isdigit():
                jobarray.append(rad)
    unfoldingjob = slurm.submit(unfoldingcmd, args.jobtag, logfile, args.queue, 1, 1, jobarray, maxtime=args.time)
    logging.info("Submitting processing job under %d", unfoldingjob)
    mergeexecutable = os.path.join(repo, "unfolding", "postprocess1D.sh")
    mergecmd = f"{mergeexecutable} {os.getcwd()}"
    logfile = "merge"
    mergejob = slurm.submit(mergecmd, f"merge_{args.jobtag}", logfile, args.queue, 1, 1, None, unfoldingjob, maxtime="2:00:00")
    logging.info("Submitting merging job under %d", mergejob)

if __name__ == "__main__":
    main()