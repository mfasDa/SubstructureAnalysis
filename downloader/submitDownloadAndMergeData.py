#! /usr/bin/env python3

import argparse
import logging
import os
import sys

from SubstructureHelpers.alien import test_alien_token, recreate_token
from SubstructureHelpers.cluster import SubmissionHostNotSupportedException, UnknownClusterException, get_cluster, is_valid_partition, PartitionException
from SubstructureHelpers.setup_logging import setup_logging
from SubstructureHelpers.slurm import submit
from SubstructureHelpers.train import AliTrainDB

class LaunchHandler:

    def __init__(self, cluster: str, outputbase: str , trainrun: int, legotrain: str):
        self.__repo = os.getenv("SUBSTRUCTURE_ROOT")
        self.__outputbase = outputbase
        self.__trainrun = None
        self.__legotrain = legotrain
        self.__cluster = cluster
        self.__partition_download = "short"
        self.__tokens = {"cert": None, "key": None}
        self.__maxtime = "01:00:00"

        pwg,trainname = self.__legotrain.split("/")
        trainDB = AliTrainDB(pwg, trainname)
        try:
            self.__trainrun = trainDB.getTrainIdentifier(trainrun)
            logging.info("Found train run: %s",  self.__trainrun)
        except AliTrainDB.UninitializedException as err:
            logging.error("%s", err)
        except AliTrainDB.TrainNotFoundException as err:
            logging.error("%s", err)

    def set_partition_for_download(self, partition: str):
        if not is_valid_partition(self.__cluster, partition):
            raise PartitionException(partition, self.__cluster)
        self.__partition_download = partition

    def set_maxtime(self, maxtime: str):
        self.__maxtime = maxtime

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
        if not os.path.exists(self.__outputbase):
            os.makedirs(self.__outputbase)
        executable = os.path.join(self.__repo, "downloader", "runDownloadAndMergeDataBatch.sh")
        jobname = f"down_{year}"
        logfile = os.path.join(self.__outputbase, "download.log")
        downloadcmd = f"{executable} {self.__repo} {self.__outputbase} {year} {self.__trainrun} {cert} {key}"
        jobid = submit(command=downloadcmd, jobname=jobname, logfile=logfile, partition=self.__partition_download, numnodes=1, numtasks=1, maxtime=self.__maxtime)
        logging.info("Submitting download job: %d",jobid)
        return jobid

if __name__ == "__main__":
    currentbase = os.getcwd()
    parser = argparse.ArgumentParser("submitDownloadAndMergeData.py", description="submitter for download and merge")
    parser.add_argument("-o", "--outputdir", metavar="VARIATION", type=str, default=currentbase, help="Output directory (default: current directory)")
    parser.add_argument("-y", "--year", metavar="YEAR", type=int,required=True, help="Year of the sample")
    parser.add_argument("-t", "--trainrun", metavar="TRAINRUN", type=int, required=True, help="Train run")
    parser.add_argument("-l", "--legotrain", metavar="LEGOTRAIN", type=str, default="PWGJE/Jets_EMC_pp", help="Name of the lego train (default: PWGJE/Jets_EMC_pp)")
    parser.add_argument("-p", "--partition", metavar="PARTITION", type=str, default="fast", help="Partition for download")
    parser.add_argument("--maxtime", metavar="MAXTIME", type=str, default="01:00:00", help="Maximum time for download job")
    parser.add_argument("-d", "--debug", action="store_true", help="Debug mode")
    args = parser.parse_args()
    setup_logging(args.debug)

    tokens = test_alien_token()
    if not tokens:
        logging.info("No valid tokens found, recreating ...")
        tokens = recreate_token()
    if not tokens:
        logging.error("Failed generating tokens ...")
        sys.exit(1)
    cert = tokens["cert"]
    key = tokens["key"]

    cluster = ""
    try:
        cluster = get_cluster()
    except SubmissionHostNotSupportedException as e:
        logging.error("Submission error: %s", e)
        sys.exit(1)
    logging.info("Submitting download on cluster %s", cluster)

    handler = LaunchHandler(cluster=cluster, outputbase=args.outputdir, trainrun=args.trainrun, legotrain=args.legotrain)
    handler.set_token(cert, key)
    handler.set_maxtime(args.maxtime)
    try:
        handler.set_partition_for_download(args.partition)
        handler.submit(args.year)
    except UnknownClusterException as e:
        logging.error("Submission error: %s", e)
        sys.exit(1)
    except PartitionException as e:
        logging.error("Submission error: %s", e)
        sys.exit(1)