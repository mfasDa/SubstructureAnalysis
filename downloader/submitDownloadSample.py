#! /usr/bin/env python3

import argparse
import logging
import os
import sys

from SubstructureHelpers.setup_logging import setup_logging
from SubstructureHelpers.alien import test_alien_token, recreate_token
from SubstructureHelpers.cluster import get_cluster, SubmissionHostNotSupportedException, PartitionException, UnknownClusterException
from downloader.mcdownloader import MCDownloadHandler

if __name__ == "__main__":
    currentbase = os.getcwd()
    parser = argparse.ArgumentParser("submitDownloadAndMergeMC.py", description="submitter for download and merge")
    parser.add_argument("-o", "--outputdir", metavar="VARIATION", type=str, default=currentbase, help="Output directory (default: current directory)")
    parser.add_argument("-s", "--sample", metavar="SAMPLE", type=str, default="", help="Copy only subsample")
    parser.add_argument("-t", "--trainrun", metavar="TRAINRUN", type=int, required=True, help="Train run (only main number)")
    parser.add_argument("-f", "--filename", metavar="FILENAME", type=str, default="AnalysisResults.root", help="File to be merged (default: AnalysisResults.root")
    parser.add_argument("-l", "--legotrain", metavar="LEGOTRAIN", type=str, default="PWGJE/Jets_EMC_pp_MC", help="Name of the lego train (default: PWGJE/Jets_EMC_pp_MC)")
    parser.add_argument("-p", "--partition", metavar="PARTITION", type=str, default="long", help="Partition for download")
    parser.add_argument("--maxtime", metavar="MAXTIME", type=str, default="01:00:00", help="Maximum time for download job")
    parser.add_argument("-c", "--check", action="store_true", help="Run check of pt-hard distribution")
    parser.add_argument("-d", "--debug", action="store_true", help="Debug mode")
    args = parser.parse_args()
    setup_logging(args.debug)

    cluster = ""
    try:
        cluster = get_cluster()
    except SubmissionHostNotSupportedException as err:
        logging.error("Submission error: %s", err)
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

    handler = MCDownloadHandler(cluster=cluster, outputbase=os.path.abspath(args.outputdir), trainrun=args.trainrun, legotrain=args.legotrain, check=args.check)
    handler.set_token(cert, key)
    handler.set_maxtime(args.maxtime)
    try:
        handler.set_partition_for_download(args.partition)
        handler.submit_sample(args.sample)
    except UnknownClusterException as err:
        logging.error("Submission error: %s", err)
        sys.exit(1)
    except PartitionException as err:
        logging.error("Submission error: %s", err)
        sys.exit(1)