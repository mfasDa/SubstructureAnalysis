#! /usr/bin/env python3

import argparse
import os

def getRunNumber(inputstring: str):
    delim = inputstring.find("_")
    if delim < 0:
        return -1
    runnostring = inputstring[:delim]
    if runnostring.isdigit():
        return int(runnostring)
    return -1

if __name__ == "__main__":
    parser = argparse.ArgumentParser("combineRuns.py", "Sort samples according to periods")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    parser.add_argument("outputdir", metavar="OUTPUTDIR", type=str, help="Output directory") 
    args = parser.parse_args()

    inputdir = args.inputdir
    outputdir = args.outputdir
    if not os.path.exists(outputdir):
        os.makedirs(outputdir, 0o755)
    runnumbers = []
    inputsamples = {key:getRunNumber(key) for key in  os.listdir(inputdir) if os.path.isdir(os.path.join(inputdir, key)) and getRunNumber(key) > -1}
    for sample,runnumber in inputsamples.items():
        outsampledir = os.path.join(outputdir, "{}".format(runnumber))
        if not runnumber in runnumbers:
            os.makedirs(outsampledir, 0o755)
            runnumbers.append(runnumber)
        insample = os.path.join(inputdir, sample) 
        outsample = os.path.join(outsampledir, sample)
        os.symlink(insample, outsample)