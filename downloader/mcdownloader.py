#! /usr/bin/env python3

import logging
import os

from SubstructureHelpers.train import AliTrainDB
from SubstructureHelpers.cluster import is_valid_partition, PartitionException
from SubstructureHelpers.slurm import submit
from merge.submitMergeRun import merge_submitter_runs

class MCDownloadHandler:

    def __init__(self, cluster: str, outputbase: str , trainrun: int, legotrain: str, mergefile: str = "AnalysisResults.root", check: bool = False, nofinal: bool = False):
        self._repo = os.getenv("SUBSTRUCTURE_ROOT")
        self._outputbase = outputbase
        self._legotrain = legotrain
        self._trainrun = None
        self._cluster = cluster
        self._partition_download = "long"
        self._mergefile = mergefile
        self._check = check
        self._tokens = {"cert": None, "key": None}
        self._maxtime = "10:00:00"
        self._nofinal = nofinal

        pwg,trainname = self._legotrain.split("/")
        trainDB = AliTrainDB(pwg, trainname)
        try:
            self._trainrun = trainDB.getTrainIdentifier(trainrun)
        except AliTrainDB.UninitializedException as e:
            logging.error("%s", e)
        except AliTrainDB.TrainNotFoundException as e:
            logging.error("%s", e)

    def set_partition_for_download(self, partition: str):
        if not is_valid_partition(self._cluster, partition):
            raise PartitionException(partition, self._cluster)
        self._partition_download = partition

    def set_maxtime(self, maxtime: str):
        self._maxtime = maxtime

    def set_token(self, cert: str, key: str):
        self._tokens["cert"] = cert
        self._tokens["key"] = key

    def submit_sample(self, sample: str, perfield: bool = False):
        if not self._trainrun:
            logging.error("Failed initializing train run")
            return
        jobid_download = self.submit_download(sample)
        if not jobid_download:
            return
        logging.info("Submitting download job with ID: %d", jobid_download)
        jobid_merge =self.submit_merge(sample, jobid_download, "2:00:00", perfield)
        logging.info("Submitting merge job with ID: %d", jobid_merge["all"])
        result_jobids = {"jobid_download": jobid_download, "jobid_merge_all": jobid_merge["all"]}
        if "pos" in jobid_merge.keys():
            logging.info("Submitting merge job (pos. field) with ID: %d", jobid_merge["pos"])
            result_jobids["jobid_merge_pos"] = jobid_merge["pos"]
        if "neg" in jobid_merge.keys():
            logging.info("Submitting merge job (neg. field) with ID: %d", jobid_merge["neg"])
            result_jobids["jobid_merge_neg"] = jobid_merge["neg"]
        return result_jobids

    def submit_download(self, sample: str) -> int:
        cert = self._tokens["cert"]
        key = self._tokens["key"]
        if not key or not cert:
            logging.error("Alien token not provided - cannot download ...")
            return None
        executable = os.path.join(self._repo, "downloader", "runDownloadAndMergeMCBatch.sh")
        jobname = f"down_{sample}"
        outputdir = os.path.join(self._outputbase, sample)
        if not os.path.exists(outputdir):
            os.makedirs(outputdir, 0o755)
        logfile = os.path.join(outputdir, "download.log")
        
        downloadcmd = f"{executable} {self._repo} {outputdir} {sample} {self._legotrain}/{self._trainrun} {cert} {key}"
        jobid = submit(command=downloadcmd, jobname=jobname, logfile=logfile, partition=self._partition_download, numnodes=1, numtasks=4, maxtime=self._maxtime)
        return jobid

    def submit_merge(self, sample: str, wait_jobid: int, maxtime: str, perfield: bool = False) -> dict:
        workdir = os.path.join(self._outputbase, sample)
        result_jobids = {}
        jobids_all = merge_submitter_runs(os.path.join(self._repo, "merge"), workdir, self._mergefile, "short", maxtime, wait_jobid, self._check)
        result_jobids["all"] = jobids_all["final"]
        if perfield:
            jobids_pos = merge_submitter_runs(os.path.join(self._repo, "merge"), workdir, self._mergefile, "short", maxtime, wait_jobid, self._check, "pos")
            jobids_neg = merge_submitter_runs(os.path.join(self._repo, "merge"), workdir, self._mergefile, "short", maxtime, wait_jobid, self._check, "neg")
            result_jobids["pos"] = jobids_pos["final"]
            result_jobids["neg"] = jobids_neg["final"]
        return result_jobids

