#! /usr/bin/env python

import argparse
import os
import sys
import subprocess

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitMergeRun.py", "Submitter for runwise merging on the 587 cluster")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    parser.add_argument("-f", "--filename", metavar="FILENAME", default="AnalysisResults.root", type=str, help="File to be merged")
    args = parser.parse_args()
    inputdir = os.path.abspath(args.inputdir)
    repo = os.path.abspath(os.path.dirname(sys.argv[0]))
    outputbase = os.path.join(inputdir, "merged")
    if not os.path.exists(outputbase):
        os.makedirs(outputbase, 0o755)
    executable = os.path.join(repo, "processMergeRun.sh")
    subprocess.call("sbatch --array=1-20 --partition=short -o {OUTPUTDIR}/joboutput_%a.log {EXECUTABLE} {INPUTDIR} {OUTPUTDIR} {FILENAME}".format(OUTPUTDIR=outputbase, EXECUTABLE=executable, INPUTDIR=inputdir, FILENAME=args.filename), shell=True)
    '''
    for ipth in range(1, 21):
        pthardbin = "%02d" %ipth
        pthinputdir = os.path.join(inputdir, pthardbin)
        pthoutputdir = os.path.join(outputbase, pthardbin)
        if not os.path.exists(pthoutputdir):
            os.makedirs(pthoutputdir)
        logfile = os.path.join(pthoutputdir, "joboutput.log")
        jobname = "merge_%s" %pthardbin
        subprocess.call("sbatch -N1 -n1 --partition=short -J %s -o %s %s %s %s %s" %(jobname, logfile, executable, pthinputdir, pthoutputdir, args.filename), shell=True)
    '''