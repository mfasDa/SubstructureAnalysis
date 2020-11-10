#! /usr/bin/env python3
import argparse
import os
import sys
import subprocess

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitGenTrainLocal.py", "Local submitter for MC gen train")
    parser.add_argument("outputdir", metavar="OUTPUTDIR", type=str, help="Output location")
    parser.add_argument("-n", "--njobs", metavar="NJOBS", type=int, default=100, help="Number of jobs")
    parser.add_argument("-c", "--configdir", metavar="CONFIGDIR", type=str, default=os.getcwd(), help="Directory with legotrain files")
    args = parser.parse_args()

    sourcedir = os.path.abspath(os.path.dirname(sys.argv[0]))
    executable = os.path.join(sourcedir, "runGetnTrainLocal.sh")
    outputdir = os.path.abspath(args.outputdir)
    logdir = os.path.join(outputdir, "logs")
    if not os.path.exists(logdir):
        os.makedirs(logdir, 0o755)
    subprocess.call("sbatch --array=1-{NJOBS} --partition=short -o {LOGDIR}/joboutput_%a.log {EXECUTABLE} {CONFIGDIR} {OUTPUTDIR}".format(NJOBS=args.njobs, LOGDIR=logdir, CONFIGDIR=args.configdir, OUTPUTDIR=outputdir), shell=True)