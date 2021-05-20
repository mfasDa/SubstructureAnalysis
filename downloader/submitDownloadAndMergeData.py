#! /usr/bin/env python3

import argparse
import logging
import os
import sys

from SubstructureHelpers.alien import test_alien_token, recreate_token
from SubstructureHelpers.slurm import submit
from SubstructureHelpers.train import AliTrainDB

class LaunchHandler:

    def __init__(self, repo: str, outputbase: str , trainrun: int, legotrain: str):
        self.__repo = repo
        self.__outputbase = outputbase
        self.__trainrun = None
        self.__legotrain = legotrain
        self.__partitionDownload = "short"
        self.__tokens = {"cert": None, "key": None}

        pwg,trainname = self.__legotrain.split("/")
        trainDB = AliTrainDB(pwg, trainname)
        try:
            self.__trainrun = trainDB.getTrainIdentifier(trainrun)
        except AliTrainDB.UninitializedException as e:
            logging.error("%s", e)
        except AliTrainDB.TrainNotFoundException as e:
            logging.error("%s", e)

    def set_partition_for_download(self, partition: str):
        if not partition in ["long", "short", "vip", "loginOnly"]:
            return
        self.__partitionDownload = partition

    def set_token(self, cert: str, key: str):
        self.__tokens["cert"] = cert
        self.__tokens["key"] = key

    def submit(self, year: int):
        if not self.__trainrun:
            logging.error("Failed initializing train run")
            return
        cert = self.__tokens["cert"]
        key = self.__tokens["key"]
        if not key or not cert:
            logging.error("Alien token not provided - cannot download ...")
            return None
        executable = os.path.join(self.__repo, "runDownloadAndMergeDataBatch.sh")
        jobname = "down_{YEAR}".format(YEAR=year)
        logfile = os.path.join(self.__outputbase, "download.log")
        downloadcmd = "{EXE} {DOWNLOADREPO} {OUTPUTDIR} {YEAR} {TRAINRUN} {ALIEN_CERT} {ALIEN_KEY}".format(EXE=executable, DOWNLOADREPO = self.__repo, OUTPUTDIR=self.__outputbase, YEAR=year, TRAINRUN=self.__trainrun, ALIEN_CERT=cert, ALIEN_KEY=key)
        jobid = submit(command=downloadcmd, jobname=jobname, logfile=logfile, partition=self.__partitionDownload, numnodes=1, numtasks=1)
        logging.info("Submitting download job: {}".format(jobid))
        return jobid

if __name__ == "__main__":
    currentbase = os.getcwd()
    repo = os.path.dirname(os.path.abspath(sys.argv[0]))
    parser = argparse.ArgumentParser("submitDownloadAndMergeData.py", description="submitter for download and merge")
    parser.add_argument("-o", "--outputdir", metavar="VARIATION", type=str, default=currentbase, help="Output directory (default: current directory)")
    parser.add_argument("-y", "--year", metavar="YEAR", type=int,required=True, help="Year of the sample")
    parser.add_argument("-t", "--trainrun", metavar="TRAINRUN", type=int, required=True, help="Train run")
    parser.add_argument("-l", "--legotrain", metavar="LEGOTRAIN", type=str, default="PWGJE/Jets_EMC_pp", help="Name of the lego train (default: PWGJE/Jets_EMC_pp)")
    parser.add_argument("-p", "--partition", metavar="PARTITION", type=str, default="short", help="Partition for download")
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

    handler = LaunchHandler(repo=repo, outputbase=args.outputdir, trainrun=args.trainrun, legotrain=args.legotrain)
    handler.set_token(cert, key)
    handler.set_partition_for_download(args.partition)
    handler.submit(args.year)