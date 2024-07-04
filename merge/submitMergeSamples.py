#! /usr/bin/env python3

import argparse
import logging
import os
import subprocess
import sys

def getRunNumber(inputstring: str):
    delim = inputstring.find("_")
    if delim < 0:
        return -1
    runnostring = inputstring[:delim]
    if runnostring.isdigit():
        return int(runnostring)
    return -1

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
    if not len(samples):
        # check if runwise
        samples = [x for x in os.listdir(os.path.abspath(args.inputdir)) if getRunNumber(x) > -1]
    for sample in samples:
        logging.info("Submitting %s ...", sample)
        fullsamplepath = os.path.join(args.inputdir, sample)
        submitcmd = f"{submitter} {fullsamplepath} -f {args.file} -p {args.partition}"
        subprocess.call(submitcmd, shell=True)