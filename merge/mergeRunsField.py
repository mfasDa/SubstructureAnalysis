#! /usr/bin/env python3
import argparse
import logging
import os
import subprocess

from SubstructureHelpers.setup_logging import setup_logging

def merge_slot(outputfile: str, inputfiles: list):
    mergedir = os.path.dirname(os.path.abspath(outputfile))
    if not os.path.exists(os.path.dirname(mergedir)):
        os.makedirs(mergedir, 0o755)
    cmd = f"hadd -f {outputfile}"
    for fl in inputfiles:
        cmd += f" {fl}"
    subprocess.call(cmd, shell=True)

def load_runs_for_field(fieldconfig: str) -> list:
    runs = []
    with open(os.path.join(os.getenv("SUBSTRUCTURE_ROOT"), "runlists_EMCAL", f"LHC18_B{fieldconfig}"), "r", encoding="utf-8") as runreader:
        for runstr in runreader:
            runs.append(int(runstr.lstrip().rstrip()))
    logging.info("Found %d runs with field configuration %s", len(runs), fieldconfig)
    return runs

def select_files(runfiles: list, fieldconfig: str ) -> list:
    runs_with_field = load_runs_for_field(fieldconfig)
    selected = [] 
    for fl in runfiles:
        logging.info("Processing file: %s", fl)
        tokens = fl.split("/")
        runnumber = int(tokens[len(tokens)-2])
        if runnumber in runs_with_field:
            logging.info("Adding run number: %d", runnumber)
            selected.append(fl)
        else:
            logging.info("Wrong field configuration found for run: %d", runnumber)
    return selected

def find_rootfiles(inputdir: str, rootfilename: str) -> list:
    allfiles = []
    for root, dirs, files in os.walk(inputdir):
        for fl in files:
            if os.path.basename(fl) == rootfilename:
                fullpath = os.path.join(root, fl)
                if "LHC" in fullpath:
                    allfiles.append(fullpath)
    return allfiles

def main():
    parser = argparse.ArgumentParser("mergeRunsField.py")
    parser.add_argument("-i", "--inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    parser.add_argument("-o", "--outputdir", metavar="OUTPUTDIR", type=str, default="default", help="Output directory (default: default:=input directory)")
    parser.add_argument("-f", "--filename", metavar="FILENAME", type=str, default="AnalysisResults.root", help="ROOT filename (default: AnalysisResults.root)")
    parser.add_argument("-b", "--bfield", metavar="BFIELD", type=str, default="neg", help="B-field (pos or neg)", choices=["pos", "neg"])
    parser.add_argument("-d", "--debug", action="store_true", help="Debug mode")
    args = parser.parse_args()

    setup_logging(args.debug)
    workdir = os.path.abspath(args.inputdir)
    allfiles = find_rootfiles(workdir, args.filename)
    selected = select_files(allfiles, args.bfield)
    outputdir = f"merged_{args.bfield}" if args.outputdir == "default" else os.path.abspath(args.outputdir)
    merge_slot(os.path.join(outputdir, args.filename), selected)

if __name__ == "__main__":
    main()