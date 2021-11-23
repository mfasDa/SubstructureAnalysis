#! /usr/bin/env python3

import argparse
import os
import sys

from SubstructureHelpers import slurm

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitUnfolding1D_local", "Submitter for 1D unfolding")
    parser.add_argument("datafile", metavar="DATFILE", type=str, help="Data input")
    parser.add_argument("mcfile", metavar="MCFILE", type=str, help="MC input")
    parser.add_argument("-w", "--workdir", metavar="WORKDIR", type=str, default=os.getcwd(), help="Working directory")
    parser.add_argument("-s", "--sysvar", metavar="SYSVAR", type=str, default="tc200", help="Systematics variation")
    parser.add_argument("-m", "--macro", metavar="MACRO", type=str, default="runCorrectionChain1DSVD_SpectrumTaskSimplePoor.sh", help="Unfolding macro")
    parser.add_argument("-j", "--jobtag", metavar="JOBTAG", type=str, default="corr1D", help="Job tag in jobname")
    parser.add_argument("-q", "--queue", metavar="QUEUE", type=str, default="short", help="Slurm queue")
    args = parser.parse_args()

    repo = os.path.abspath(os.path.dirname(sys.argv[0]))
    unfoldingexecutable = os.path.join(repo, "runUnfolding1D_local.sh")
    unfoldingcmd="{EXE} {WDIR} {DATAFILE} {MCFILE} {SYSVAR} {MACRO}".format(EXE=unfoldingexecutable, WDIR=args.workdir, DATAFILE=args.datafile, MCFILE=args.mcfile, SYSVAR=args.sysvar, MACRO=args.macro)
    logfile="joboutput_R0%a.log"
    os.chdir(args.workdir)
    unfoldingjob = slurm.submit(unfoldingcmd, args.jobtag, logfile, args.queue, 1, 1, [2, 6])
    print("Submitting processing job under %d" %unfoldingjob)
    mergeexecutable = os.path.join(repo, "postprocess1D.sh")
    mergecmd = "{EXE} {WORKDIR}".format(EXE=mergeexecutable, WORKDIR=os.getcwd())
    logfile = "merge"
    mergejob = slurm.submit(mergecmd, "merge_{TAG}".format(TAG=args.jobtag), logfile, args.queue, 1, 1, None, unfoldingjob)
    print("Submitting merging job under %d" %mergejob)