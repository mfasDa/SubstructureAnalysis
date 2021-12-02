#! /usr/bin/env python3
import argparse
import logging
import os
from posixpath import basename

from getRawCountsFromLogbookXML import process_convert

if __name__ == "__main__":
    parser = argparse.ArgumentParser("getRawCountsFromLogbookXML.py", description="Converting logbook trigger xml to csv")
    parser.add_argument("-w", "--workdir", metavar="WORKDIR", type=str, default=os.getcwd(), help="working directory")
    parser.add_argument("-d", "--debug", action="store_true", help="Print debug information")
    args = parser.parse_args()

    loglevel = logging.INFO
    if args.debug:
        loglevel = logging.DEBUG
    logging.basicConfig(format='[%(levelname)s]: %(message)s', level=loglevel)

    workdir = os.path.abspath(args.workdir)
    xmlfiles = [x for x in os.listdir(workdir) if os.path.isfile(x) and "xml" in x]
    for fl in xmlfiles:
        basexml = os.path.basename(fl)
        period = basexml[:basexml.find(".")]
        basecsv = "{}.csv".format(period)
        inputfile = os.path.join(workdir, basexml)
        outputfile = os.path.join(workdir, basecsv)
        logging.info("Converting period %s: %s -> %s", period, inputfile, outputfile)
        process_convert(inputfile, outputfile)