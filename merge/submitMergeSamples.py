#! /usr/bin/env python3

import argparse
import logging
import os
import subprocess
import sys

if __name__ == "__main__":
    scriptdir = os.path.dirname(os.path.abspath(sys.argv[0]))
    submitter = os.path.join(scriptdir, "submitMergeRun.py")
    parser = argparse.ArgumentParser("submitMergeSamples.py", description="Launch merging of single dataset merging")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    parser.add_argument("-f", "--file", metavar="FILE", type=str, default="AnalysisResults.root", help="")
    parser.add_argument("-p", "--partition", metavar="PARTITION", type=str, default="vip", help="Partition of the 587 cluster")
    parser.add_argument("-d", "--debug", action="store_true", help="Debug mode")
    args = parser.parse_args()

    loglevel = logging.INFO
    if args.debug:
        loglevel = logging.DEBUG
    logging.basicConfig(format="%(levelname)s: %(message)s", level=loglevel)

    samples = [x for x in os.listdir(os.path.abspath(args.inputdir)) if "LHC" in x]
    for sample in samples:
        logging.info("Submitting %s ...", sample)
        fullsamplepath = os.path.join(args.inputdir, sample)
        submitcmd = "{EXE} {SAMPLEDIR} -f {FILE} -p {PARTITION}".format(EXE=submitter, SAMPLEDIR=fullsamplepath, FILE=args.file, PARTITION=args.partition)
        subprocess.call(submitcmd, shell=True)