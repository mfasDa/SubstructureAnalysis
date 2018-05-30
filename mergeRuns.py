#! /usr/bin/env python
from __future__ import print_function
import logging
import os, sys

def ExecMerge(outputfile, filelist):
    logging.info("Merging output to %s" %outputfile)
    mergecommand = "hadd -f %s" %(outputfile)
    for gridfile in filelist:
        logging.info("Adding %s" %gridfile)
        mergecommand += " %s" %(gridfile)
    os.system(mergecommand)

def GetFilelist(inputpath, filename):
    logging.debug("walking %s" %inputpath)
    result = []
    for root, dirs, files in os.walk(inputpath):
        for f in files:
            if filename in f:
                result.append(os.path.join(root, f))
    return result

def DoMerge(inputpath, filename):
    mergedir = "%s/merged" %(inputpath)
    if not os.path.exists(mergedir):
        os.makedirs(mergedir, 0755)

    for pthard in sorted(os.listdir(inputpath)):
        if not pthard.isdigit():
            continue
        logging.info("Merging all file from pt-hard bin %s" %(pthard))
        outputdir = os.path.join(mergedir, pthard)
        if not os.path.exists(outputdir):
            os.makedirs(outputdir, 0755)
        ExecMerge(os.path.join(outputdir, filename), GetFilelist(os.path.join(inputpath, pthard), filename))
    logging.info("Done")

if __name__ == "__main__":
    logging.basicConfig(format="[%(levelname)s] %(message)s", level = logging.INFO)
    inputpath = sys.argv[1]
    rootfile = "AnalysisResults.root"
    if len(sys.argv) > 2:
        rootfile = sys.argv[2]
    DoMerge(inputpath, rootfile)
