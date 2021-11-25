#! /usr/bin/env python3

import argparse
import logging
import os
import sys

from SubstructureHelpers.alien import test_alien_token, recreate_token
from SubstructureHelpers.slurm import submit
from SubstructureHelpers.train import AliTrainDB
from merge.submitMergeRun import merge_submitter_runs
from merge.submitMergeMCDatasets import merge_submitter_datasets

class LaunchHandler:

    def __init__(self, outputbase: str , trainrun: int, legotrain: str, mergefile: str = "AnalysisResults.root", check: bool = False):
        self.__repo = os.getenv("SUBSTRUCTURE_ROOT")
        self.__outputbase = outputbase
        self.__legotrain = legotrain
        self.__trainrun = None
        self.__partitionDownload = "long"
        self.__mergefile = mergefile
        self.__check = check
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

    def submit(self, year: int, subsample: str = ""):
        if not self.__trainrun:
            logging.error("Failed initializing train run")
            return
        mcsamples = {2016: ["LHC19a1a_1", "LHC19a1a_2", "LHC19a1b_1", "LHC19a1b_2", "LHC19a1c_1", "LHC19a1c_2"], 2017: ["LHC18f5_1", "LHC18f5_2"], 2018: ["LHC19d3_1", "LHC19d3_1_extra", "LHC19d3_2", "LHC19d3_2_extra"]}
        if not year in mcsamples.keys():
            logging.error("No sample or year %d", year)
        if len(subsample):
            if not subsample in mcsamples[year]:
                logging.error("Requested subsample %s not found for year %d ...", subsample, year)
                return
        jobids_merge = []
        for sample in mcsamples[year]:
            select = False
            if len(subsample):
                if sample == subsample:
                    select = True
            else:
                select = True
            if not select:
                continue
            jobid_download = self.submit_download_MC(sample)
            if not jobid_download:
                return
            logging.info("Submitting download job with ID: {}".format(jobid_download))
            jobids_merge.append(self.submit_merge(sample, jobid_download))
        if len(jobids_merge) > 1:
            # if we have at least 2 subsamples submit also period merging
            self.submit_merge_samples(jobids_merge)

    def submit_download_MC(self, sample: str) -> int:
        cert = self.__tokens["cert"]
        key = self.__tokens["key"]
        if not key or not cert:
            logging.error("Alien token not provided - cannot download ...")
            return None
        executable = os.path.join(self.__repo, "downloader", "runDownloadAndMergeMCBatch.sh")
        jobname = "down_{SAMPLE}".format(SAMPLE=sample)
        outputdir = os.path.join(self.__outputbase, sample)
        if not os.path.exists(outputdir):
            os.makedirs(outputdir, 0o755)
        logfile = os.path.join(outputdir, "download.log")
        
        downloadcmd = "{EXE} {DOWNLOADREPO} {OUTPUTDIR} {DATASET} {LEGOTRAIN}/{TRAINID} {ALIEN_CERT} {ALIEN_KEY}".format(EXE=executable, DOWNLOADREPO=self.__repo, OUTPUTDIR=outputdir, DATASET=sample, LEGOTRAIN=self.__legotrain, TRAINID=self.__trainrun, ALIEN_CERT=cert, ALIEN_KEY=key)
        jobid = submit(command=downloadcmd, jobname=jobname, logfile=logfile, partition=self.__partitionDownload, numnodes=1, numtasks=4)
        return jobid

    def submit_merge(self, sample: str, wait_jobid: int) -> int:
        workdir = os.path.join(self.__outputbase, sample)
        jobids = merge_submitter_runs(os.path.join(self.__repo, "merge"), workdir, self.__mergefile, "short", wait_jobid, self.__check)
        return jobids["final"]

    def submit_merge_samples(self, wait_jobids: list) -> int:
        merge_submitter_datasets(os.path.join(self.__repo, "merge"), self.__outputbase, self.__mergefile, "short", wait_jobids, self.__check)

if __name__ == "__main__":
    currentbase = os.getcwd()
    parser = argparse.ArgumentParser("submitDownloadAndMergeMC.py", description="submitter for download and merge")
    parser.add_argument("-o", "--outputdir", metavar="VARIATION", type=str, default=currentbase, help="Output directory (default: current directory)")
    parser.add_argument("-y", "--year", metavar="YEAR", type=int,required=True, help="Year of the sample")
    parser.add_argument("-t", "--trainrun", metavar="TRAINRUN", type=int, required=True, help="Train run (only main number)")
    parser.add_argument("-f", "--filename", metavar="FILENAME", type=str, default="AnalysisResults.root", help="File to be merged (default: AnalysisResults.root")
    parser.add_argument("-l", "--legotrain", metavar="LEGOTRAIN", type=str, default="PWGJE/Jets_EMC_pp_MC", help="Name of the lego train (default: PWGJE/Jets_EMC_pp_MC)")
    parser.add_argument("-s", "--subsample", metavar="SUBSAMPLE", type=str, default="", help="Copy only subsample")
    parser.add_argument("-p", "--partition", metavar="PARTITION", type=str, default="long", help="Partition for download")
    parser.add_argument("-c", "--check", action="store_true", help="Run check of pt-hard distribution")
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

    handler = LaunchHandler(outputbase=args.outputdir, trainrun=args.trainrun, legotrain=args.legotrain, check=args.check)
    handler.set_token(cert, key)
    handler.set_partition_for_download(args.partition)
    handler.submit(args.year)
