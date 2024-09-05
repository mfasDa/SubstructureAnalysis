#! /usr/bin/env python3

import argparse
import os

from ROOT import RDataFrame, TFile, gDirectory
from pandas import DataFrame as pdf

def find_pthard_bin(histo) -> int:
    pthardbin = 0
    for binID in range(0, histo.GetNbinsX()):
        if histo.GetBinContent(binID+1):
            pthardbin = binID
            break
    return pthardbin

def get_value(histo, pthardbin: int) -> float:
    return histo.GetBinContent(pthardbin + 1)

def convert_mcset(filename: str, outputdir: str):
    reader = TFile.Open(filename, "READ")
    keys = reader.GetListOfKeys()
    treename = ""
    crosssection = 0
    ntrials = 0
    pthardbin = -1
    for key in keys:
        if "Tree" in key.GetName():
            treename = key.GetName()
        elif "JetSubstructure" in key.GetName():
            reader.cd(key.GetName())
            histos = gDirectory.GetListOfKeys().At(0).ReadObj()
            xsechist = histos.FindObject("fHistXsection")
            ntrialshist = histos.FindObject("fHistTrials")
            pthardbin = find_pthard_bin(ntrialshist)
            print(f"Found pt-hard bin: {pthardbin}")
            crosssection = get_value(xsechist, pthardbin)
            ntrials = get_value(ntrialshist, pthardbin)
    reader.Close()
    weight = crosssection / ntrials

    df = RDataFrame(treename, filename)
    df_with_infos = df.Define("weight", f"{weight}").Define("xsec", f"{crosssection}").Define("ntrials", f"{ntrials}").Define("pthardbin", f"{pthardbin}")
    
    pandasdf = pdf(df_with_infos.AsNumpy())
    pandasdf.to_csv(os.path.join(os.path.abspath(outputdir), f"mcjetsetpthard_{pthardbin}.csv.gzip"), compression="gzip")

def main():
    parser = argparse.ArgumentParser("extractResponse.py") 
    parser.add_argument("filename", metavar="FILENAME", type=str, help="File to process")
    parser.add_argument("-o", "--outputdir", metavar="OUTPUTDIR", type=str, default="", help="Output directory")
    args = parser.parse_args()

    outputdir = args.outputdir
    if outputdir:
        outputdir = os.path.dirname(os.path.abspath)
    convert_mcset(args.filename, outputdir)

if __name__ == "__main__":
    main()