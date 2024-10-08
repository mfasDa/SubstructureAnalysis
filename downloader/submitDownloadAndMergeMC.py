#! /usr/bin/env python3

import argparse
import logging
import os
import sys

from SubstructureHelpers.setup_logging import setup_logging
from SubstructureHelpers.alien import test_alien_token, recreate_token
from SubstructureHelpers.cluster import get_cluster, SubmissionHostNotSupportedException, PartitionException, UnknownClusterException
from merge.submitMergeMCDatasets import merge_submitter_datasets
from downloader.mcdownloader import MCDownloadHandler

class MultiSampleDownloadHandler(MCDownloadHandler):

    def submit(self, year: int, subsample: str = "", bfield: bool = False):
        if not self._trainrun:
            logging.error("Failed initializing train run")
            return
        mcsamples = {2016: ["LHC19a1a_1", "LHC19a1a_2", "LHC19a1b_1", "LHC19a1b_2", "LHC19a1c_1", "LHC19a1c_2"], 2017: ["LHC18f5_1", "LHC18f5_2"], 2018: ["LHC19d3_1", "LHC19d3_1_extra", "LHC19d3_2", "LHC19d3_2_extra"], 20175: ["LHC18b8_cent_woSDD", "LHC18b8_fast"]}
        if not year in mcsamples.keys():
            logging.error("No sample or year %d", year)
        if len(subsample):
            if not subsample in mcsamples[year]:
                logging.error("Requested subsample %s not found for year %d ...", subsample, year)
                return
        jobids_merge = []
        jobids_merge_pos = []
        jobids_merge_neg = []
        for sample in mcsamples[year]:
            require_field_merge = bfield and year == 2018
            select = False
            if len(subsample):
                if sample == subsample:
                    select = True
            else:
                select = True
            if not select:
                continue
            jobids_sample = self.submit_sample(sample, require_field_merge)
            jobids_merge.append(jobids_sample["jobid_merge_all"])
            if "jobid_merge_pos" in jobids_sample.keys():
                jobids_merge_pos.append(jobids_sample["jobid_merge_pos"])
            if "jobid_merge_neg" in jobids_sample.keys():
                jobids_merge_neg.append(jobids_sample["jobid_merge_neg"])
        if len(jobids_merge) > 1:
            # if we have at least 2 subsamples submit also period merging
            self.submit_merge_samples(jobids_merge, "2:00:00", "all")
        if len(jobids_merge_pos) > 1:
            # if we have at least 2 subsamples submit also period merging
            self.submit_merge_samples(jobids_merge_pos, "2:00:00", "pos")
        if len(jobids_merge_neg) > 1:
            # if we have at least 2 subsamples submit also period merging
            self.submit_merge_samples(jobids_merge_neg, "2:00:00", "neg")

    def submit_merge_samples(self, wait_jobids: list, maxtime: str, field: str = "all") -> int:
        merge_submitter_datasets(os.path.join(self._repo, "merge"), self._outputbase, self._mergefile, "short", maxtime, wait_jobids, self._check, self._nofinal, field)

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
    parser.add_argument("--maxtime", metavar="MAXTIME", type=str, default="01:00:00", help="Maximum time for download job")
    parser.add_argument("-c", "--check", action="store_true", help="Run check of pt-hard distribution")
    parser.add_argument("-n", "--nofinal", action="store_true", help="Do not launch final merge job (i.e. for trees)")
    parser.add_argument("-b", "--bfield", action="store_true", help="Per b-field")
    parser.add_argument("-d", "--debug", action="store_true", help="Debug mode")
    args = parser.parse_args()
    setup_logging(args.debug)

    cluster = ""
    try:
        cluster = get_cluster()
    except SubmissionHostNotSupportedException as e:
        logging.error("Submission error: %s", e)
        sys.exit(1)
    logging.info("Submitting download on cluster %s", cluster)

    tokens = test_alien_token()
    if not tokens:
        logging.info("No valid tokens found, recreating ...")
        tokens = recreate_token()
    if not tokens:
        logging.error("Failed generating tokens ...")
        sys.exit(1)
    cert = tokens["cert"]
    key = tokens["key"]

    handler = MultiSampleDownloadHandler(cluster=cluster, outputbase=args.outputdir, trainrun=args.trainrun, legotrain=args.legotrain, check=args.check, nofinal=args.nofinal)
    handler.set_token(cert, key)
    handler.set_maxtime(args.maxtime)
    try:
        handler.set_partition_for_download(args.partition)
        handler.submit(args.year, bfield=args.bfield)
    except UnknownClusterException as e:
        logging.error("Submission error: %s", e)
        sys.exit(1)
    except PartitionException as e:
        logging.error("Submission error: %s", e)
        sys.exit(1)
