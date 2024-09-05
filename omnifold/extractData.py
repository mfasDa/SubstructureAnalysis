#! /usr/bin/env python3

import argparse
import os

from ROOT import RDataFrame, TFile, gDirectory
from pandas import DataFrame as pdf

def convert_dataset(filename: str, outputdir: str, tag: str = ""):
    reader = TFile.Open(filename, "READ")
    keys = reader.GetListOfKeys()
    treename = ""
    for key in keys:
        if "Tree" in key.GetName():
            treename = key.GetName()
            break
    reader.Close()

    df = RDataFrame(treename, filename)
    
    pandasdf = pdf(df.AsNumpy())
    filemain = "datajetset"
    if tag:
        filemain += f"_{tag}"
    pandasdf.to_csv(os.path.join(os.path.abspath(outputdir), f"{filemain}.csv.gzip"), compression="gzip")

def main():
    parser = argparse.ArgumentParser("extractResponse.py") 
    parser.add_argument("filename", metavar="FILENAME", type=str, help="File to process")
    parser.add_argument("-t", "--tag", metavar="TAG", type=str, default="", help="Tag added to the output csv file (optional)")
    parser.add_argument("-o", "--outputdir", metavar="OUTPUTDIR", type=str, default="", help="Output directory")
    args = parser.parse_args()

    outputdir = args.outputdir
    if not outputdir:
        outputdir = os.path.dirname(os.path.abspath(args.filename))
    convert_dataset(args.filename, outputdir, args.tag)

if __name__ == "__main__":
    main()