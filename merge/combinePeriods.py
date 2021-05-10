#! /usr/bin/env python3

import argparse
import os

if __name__ == "__main__":
    parser = argparse.ArgumentParser("combinePeriods.py", "Sort samples according to periods")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    parser.add_argument("outputdir", metavar="OUTPUTDIR", type=str, help="Output directory") 
    args = parser.parse_args()

    inputdir = args.inputdir
    outputdir = args.outputdir
    if not os.path.exists(outputdir):
        os.makedirs(outputdir, 0o755)
    datasets = []
    inputsamples = [x for x in os.listdir(inputdir) if "LHC in x"]
    for sample in inputsamples:
        dataset = sample.split("_")[0]
        outsampledir = os.path.join(outputdir, dataset)
        if not dataset in datasets:
            os.makedirs(outsampledir, 0o755)
            datasets.append(dataset)
        insample = os.path.join(inputdir, sample) 
        outsample = os.path.join(outsampledir, sample)
        os.symlink(insample, outsample)