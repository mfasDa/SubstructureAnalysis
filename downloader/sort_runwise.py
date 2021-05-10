#! /usr/bin/env python3

import argparse
import logging
import os
import shutil
import sys

class RunSorter:

    def __init__(self, inputdir: str, outputdir: str, rootfile: str):
        self.__inputdir = inputdir
        self.__outputdir = outputdir
        self.__rootfile = rootfile
        self.__tag = self.__read_tag(os.path.basename(self.__inputdir))
        print("determined tag {}".format(self.__tag))

    def __read_tag(self, production: str) -> str:
        print("Creating tag from {}".format(production))
        tokens = production.rstrip().lstrip().split("_")
        tag = tokens[len(tokens)-1]
        if tag == "extra":
            tag = "{}_{}".format(tokens[len(tokens)-2], tokens[len(tokens)-1])
        return tag

    def has_rundir(self, run: int) -> str:
        teststring = "{}_{}".format(run, self.__tag)
        for testperiod in os.listdir(self.__outputdir):
            if testperiod ==  teststring:
                return testperiod
        return ""

    def init_run(self, run: str) -> str:
        logging.info("Initializing directory structure for period %s", run)
        rundir = os.path.join(self.__outputdir, "{}_{}".format(run, self.__tag))
        os.makedirs(rundir, 0o755)
        for ipth in range(1, 21):
            pthardbindir = os.path.join(rundir, "%02d" %ipth)
            os.makedirs(pthardbindir, 0o755)
        return os.path.basename(rundir)

    def copy_to_target(self, inputfile: str, run: int, pthardbin: int):
        targetdir = self.has_rundir(run)
        logging.debug("Inputfile: %s, run: %s, pthardin %s, targetdir %s", inputfile, run, pthardbin, targetdir)
        if not len(targetdir):
            targetdir = self.init_run(run)
        outputdir = os.path.join(self.__outputdir, targetdir, "%02d" %pthardbin)
        if not os.path.exists(outputdir):
            os.makedirs(outputdir, 0o755)
        outputfile = os.path.join(outputdir, os.path.basename(inputfile))
        logging.debug("Copying %s to %s", inputfile, outputfile)
        shutil.copyfile(inputfile, outputfile)

    def sort(self):
        for pthardbin in range(1, 21):
            pthardbindir = os.path.join(self.__inputdir, "%02d" %pthardbin)
            for run in os.listdir(pthardbindir):
                inputfile = os.path.join(pthardbindir, run, self.__rootfile)
                if not os.path.exists(inputfile):
                    continue
                self.copy_to_target(inputfile, int(run), int(pthardbin))

if __name__ == "__main__":
    repo = os.path.dirname(os.path.dirname(os.path.abspath(sys.argv[0])))
    parser = argparse.ArgumentParser("sort_runs.py", description="Sort runs according to periods")
    parser.add_argument("-i", "--inputdir", metavar="INPUTDIR", type=str, required=True, help="Input directory with subsamples")
    parser.add_argument("-o", "--outputdir", metavar="OUTPUTDIR", type=str, required=True, help="Output directory for sorted samples")
    parser.add_argument("-r", "--rootfile", metavar="ROOTFILE", type=str, default="AnalysisResults.root", help="Rootfile to be sorted")
    parser.add_argument("-d", "--debug", action="store_true", help="Debug mode")
    args = parser.parse_args()

    loglevel = logging.INFO
    if args.debug:
        loglevel = logging.DEBUG
    logging.basicConfig(format="[%(levelname)s]: %(message)s", level=loglevel)

    subsamples = [x for x in os.listdir(args.inputdir) if "LHC" in x]
    for subsample in subsamples:
        logging.info("Sorting subsample %s", subsample)
        if not os.path.exists(args.outputdir):
            os.makedirs(args.outputdir, 0o755)
        sorter = RunSorter(os.path.join(args.inputdir, subsample), args.outputdir, args.rootfile)
        sorter.sort()
