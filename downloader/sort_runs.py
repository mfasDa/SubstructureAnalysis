#! /usr/bin/env python3

import argparse
import logging
import os
import shutil
import sys

class SampleDB:

    def __init__(self, basedir):
        self.__basedir = os.path.join(basedir, "runlists_EMCAL")
        self.__runlists = {}
        self.__initialize()

    def find_period(self, run: int) -> str:
        period = ""
        for testperiod, runs in self.__runlists.items():
            if run in runs:
                period = testperiod
                break
        return period

    def __initialize(self):
        nruns = 0
        for period in os.listdir(self.__basedir):
            logging.debug("Adding runs for period %s", period)
            self.__runlists[period] = self.__read_runlist(os.path.join(self.__basedir, period))
            nruns += len(self.__runlists[period])
        logging.info("Sample DB initialized with %d runs", nruns)

    def __read_runlist(self, filename: str) -> list:
        runs = []
        with open(filename, "r") as runreader:
            for line in runreader:
                for en in line.split(","):
                    runstring = en.lstrip().rstrip()
                    if not len(runstring):
                        continue
                    runs.append(int(runstring))
        return runs

class SampleSorter:

    def __init__(self, inputdir: str, outputdir: str, rootfile: str, sampledb: SampleDB):
        self.__inputdir = inputdir
        self.__outputdir = outputdir
        self.__rootfile = rootfile
        self.__tag = self.__read_tag(os.path.basename(self.__inputdir))
        print("determined tag {}".format(self.__tag))
        self.__sampleDB = sampledb

    def __read_tag(self, production: str) -> str:
        print("Creating tag from {}".format(production))
        tokens = production.rstrip().lstrip().split("_")
        tag = tokens[len(tokens)-1]
        if tag == "extra":
            tag = "{}_{}".format(tokens[len(tokens)-2], tokens[len(tokens)-1])
        return tag

    def has_period(self, period: str) -> str:
        teststring = "{}_{}".format(period, self.__tag)
        for testperiod in os.listdir(self.__outputdir):
            if testperiod ==  teststring:
                return testperiod
        return ""

    def init_period(self, period: str) -> str:
        logging.info("Initializing directory structure for period %s", period)
        perioddir = os.path.join(self.__outputdir, "{}_{}".format(period, self.__tag))
        os.makedirs(perioddir, 0o755)
        for ipth in range(1, 21):
            pthardbindir = os.path.join(perioddir, "%02d" %ipth)
            os.makedirs(pthardbindir, 0o755)
        return os.path.basename(perioddir)

    def copy_to_target(self, inputfile: str, run: int, pthardbin: int):
        period = self.__sampleDB.find_period(run)
        if not len(period):
            logging.error("No period found for run %d", run)
            return
        targetdir = self.has_period(period)
        logging.debug("Inputfile: %s, run: %s, pthardin %s, period %s, targetdir %s", inputfile, run, pthardbin, period, targetdir)
        if not len(targetdir):
            targetdir = self.init_period(period)
        outputdir = os.path.join(self.__outputdir, targetdir, "%02d" %pthardbin, "%d" %run)
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

    sampeldb = SampleDB(repo)
    subsamples = [x for x in os.listdir(args.inputdir) if "LHC" in x]
    for subsample in subsamples:
        logging.info("Sorting subsample %s", subsample)
        if not os.path.exists(args.outputdir):
            os.makedirs(args.outputdir, 0o755)
        sorter = SampleSorter(os.path.join(args.inputdir, subsample), args.outputdir, args.rootfile, sampeldb)
        sorter.sort()
