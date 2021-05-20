#! /usr/bin/env python3

import argparse
import logging
import os
import sys

from SubstructureHelpers.alien import test_alien_token, recreate_token
from SubstructureHelpers.slurm import submit

class SampleDB:

    class YearNotFoundException(Exception):

        def __init__(self, year):
            self.__year = year

        def __str__(self):
            return "No samples available for year {}".format(self.__year)


    def __init__(self):
        self.__datasamples = {2016: {"LHC16h": "pass1", "LHC16i": "pass1", "LHC16j": "pass1", "LHC16k": "pass2", "LHC16l": "pass2", 
                                     "LHC16o": "pass1", "LHC16p": "pass1"},
                              2017: {"LHC17h": "pass1", "LHC17i": "pass1", "LHC17j": "pass1", "LHC17k": "pass1", "LHC17l": "pass1",
                                     "LHC17m": "pass1", "LHC17o": "pass1", "LHC17r": "pass1"},
                              2018: {"LHC18d": "pass1", "LHC18e": "pass1", "LHC18f": "pass1", "LHC18g": "pass1", "LHC18h": "pass1",
                                     "LHC18i": "pass1", "LHC18j": "pass1", "LHC18k": "pass1", "LHC18l": "pass1", "LHC18m": "pass1_withTRDtracking",
                                     "LHC18n": "pass1", "LHC18o": "pass1", "LHC18p": "pass1"}
        }

    def getDataSamples(self, year):
        if not year in self.__datasamples.keys():
            raise SampleDB.YearNotFoundException(year)
        return self.__datasamples[year]


class LaunchHandler:

    class SampleException(Exception):

        def __init__(self, year):
            self.__year = year

        def __str__(self):
            return "Failed retrieving sample for {YEAR}".format(YEAR=self.__year)

    def __init__(self, outputbase: str, isData: bool, year: int, trainrun: int, legotrain: str):
        self.__repo = os.getenv("SUBSTRUCTURE_ROOT")
        self.__outputbase = outputbase
        self.__year = year
        self.__trainrun = trainrun
        self.__legotrain = legotrain
        self.__aodset = None
        self.__filename = None
        self.__partitionDownload = "short"
        self.__tokens = {"cert": None, "key": None}
        self.__sampleDB = SampleDB()

    def set_partition_for_download(self, partition: str):
        if not partition in ["long", "short", "vip", "loginOnly"]:
            return
        self.__partitionDownload = partition

    def set_filename(self, filename):
        self.__filename = filename

    def set_aodprod(self, aodset):
        self.__aodset = aodset

    def set_token(self, cert: str, key: str):
        self.__tokens["cert"] = cert
        self.__tokens["key"] = key

    def submit(self):
        if not self.__trainrun:
            logging.error("Failed initializing train run")
            return
        cert = self.__tokens["cert"]
        key = self.__tokens["key"]
        if not key or not cert:
            logging.error("Alien token not provided - cannot download ...")
            return None
        executable = os.path.join(self.__repo, "downloader", "runDownloadRunwise.sh")
        samples = {}
        try:
            samples = self.__sampleDB.getDataSamples(self.__year)
        except SampleDB.YearNotFoundException as e:
            logging.error(e)
            raise LaunchHandler.SampleException(self.__year)
        jobids = {}
        for period,recpass in samples.items():
            logging.info("Downloading period %s, reconstruction pass %s", period, recpass)
            perioddir = os.path.join(self.__outputbase, period)
            if not os.path.exists(perioddir):
                os.makedirs(perioddir, 0o755)
            jobname = "down_{PERIOD}".format(PERIOD=period)
            logfile = os.path.join(perioddir, "download.log")
            filenamestr = self.__filename if self.__filename != None else "NONE"
            aodprodstr = self.__aodset if self.__aodset != None else  "NONE"
            downloadcmd = "{EXE} {DOWNLOADREPO} {ALIEN_CERT} {ALIEN_KEY} {OUTPUTDIR} {TRAINRUN} {LEGOTRAIN} {DATASET} {RECPASS} {AODPROD} {FILENAME}".format(EXE=executable, DOWNLOADREPO = self.__repo, ALIEN_CERT=cert, ALIEN_KEY=key, OUTPUTDIR=perioddir, TRAINRUN=self.__trainrun, LEGOTRAIN=self.__legotrain, DATASET=period, RECPASS=recpass, AODPROD=aodprodstr, FILENAME=filenamestr)
            logging.debug("Download command: %s", downloadcmd)
            jobid = submit(command=downloadcmd, jobname=jobname, logfile=logfile, partition=self.__partitionDownload, numnodes=1, numtasks=1)
            logging.info("Submitting download job for period {}: {}".format(period, jobid))
            jobids[period] = jobid
        return jobids

if __name__ == "__main__":
    currentbase = os.getcwd()
    parser = argparse.ArgumentParser("submitDownloadAndMergeData.py", description="submitter for download and merge")
    parser.add_argument("-o", "--outputdir", metavar="OUTPUTDIR", type=str, default=currentbase, help="Output directory (default: current directory)")
    parser.add_argument("-y", "--year", metavar="YEAR", type=int,required=True, help="Year of the sample")
    parser.add_argument("-t", "--trainrun", metavar="TRAINRUN", type=int, required=True, help="Train run")
    parser.add_argument("-l", "--legotrain", metavar="LEGOTRAIN", type=str, default="PWGJE/Jets_EMC_pp", help="Name of the lego train (default: PWGJE/Jets_EMC_pp)")
    parser.add_argument("-p", "--partition", metavar="PARTITION", type=str, default="short", help="Partition for download")
    parser.add_argument("-f", "--filename", metavar="FILENAME", type=str, default="", help="Filename to download (default: AnalysisResults.root")
    parser.add_argument("-a", "--aodprod", metavar="AODPROD", type=str, default="", help="AOD production (if dedicated)")
    parser.add_argument("-d", "--debug", action="store_true", help="Debug mode")
    args = parser.parse_args()

    loglevel = logging.INFO
    if args.debug:
        loglevel = logging.DEBUG
    logging.basicConfig(format="[%(levelname)s]: %(message)s", level=loglevel)

    tokens = test_alien_token()
    if not len(tokens):
        logging.info("No valid tokens found, recreating ...")
        tokens = recreate_token()
    if not len(tokens):
        logging.error("Failed generating tokens ...")
        sys.exit(1)
    cert = tokens["cert"]
    key = tokens["key"]

    handler = LaunchHandler(outputbase=args.outputdir, year=args.year, isData=True, trainrun=args.trainrun, legotrain=args.legotrain)
    handler.set_token(cert, key)
    handler.set_partition_for_download(args.partition)
    if len(args.filename):
        handler.set_filename(args.filename)
    if len(args.aodprod):
        handler.set_aodprod(args.aodprod)
    handler.submit()