#! /usr/bin/env python3

import argparse
import os

from SubstructureHelpers import slurm

def selectWorkdir(dirname: str):
    if not os.path.isdir(dirname):
        return False
    filebase = os.path.basename(dirname)
    if not "LHC" in filebase and not "merged" in filebase:
        return False
    if not os.path.isfile(os.path.join(dirname, "AnalysisResults.root")):
        return False
    return True

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitNormalizedRaw", "Submitter for raw spectra extraction")
    parser.add_argument("-w", "--workdir", metavar="WORKDIR", type=str, default=os.getcwd(), help="Working directory")
    parser.add_argument("-t", "--type", metavar="JETTYPE", type=str, default="full", help="jettype")
    parser.add_argument("-s", "--sysvar", metavar="SYSVAR", type=str, default="tc200", help="Systematics variation")
    parser.add_argument("-m", "--mcfile", metavar="MCFILE", type=str, default="NONE", help="MC file for trigger efficiency correction")
    parser.add_argument("-q", "--queue", metavar="QUEUE", type=str, default="short", help="Slurm queue")
    args = parser.parse_args()

    repo = os.getenv("SUBSTRUCTURE_ROOT")
    executable = os.path.join(repo, "unfolding", "runNormalizedRawSpectrum.sh")
    workdir = os.path.abspath(args.workdir)
    directories = [x for x in os.listdir(workdir) if selectWorkdir(os.path.join(workdir, x))]
    print("Selected {} working directories".format(len(directories)))

    for jdir in directories:
        print("Submitting {}".format(jdir))
        jobdir = os.path.join(workdir, jdir)
        jobtag = "nr_{}".format(jdir)
        logfile = os.path.join(jobdir, "normraw.log")
        runcmd = "{EXE} {REPO} {WORKDIR} {JETTYPE} {SYSVAR} {MCFILE}".format(EXE=executable, REPO=repo, WORKDIR=jobdir, JETTYPE=args.type, SYSVAR=args.sysvar, MCFILE=args.mcfile)
        slurm.submit(runcmd, jobtag, logfile, args.queue)