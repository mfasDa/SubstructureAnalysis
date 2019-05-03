#! /usr/bin/env python3
import os
import subprocess
import sys

if __name__ == "__main__":
    childmap16 = {"LHC16h": "child_4", "LHC16i": "child_5", "LHC16j": "child_6", 
                  "LHC16k": "child_7", "LHC16l": "child_8", "LHC16o": "child_9", 
                  "LHC16p": "child_10"}
    childmap17 = {"LHC17h": "child_4", "LHC17i": "child_5", "LHC17j": "child_6", 
                  "LHC17k": "child_7", "LHC17l": "child_8", "LHC17m": "child_9", 
                  "LHC17o": "child_10", "LHC17r": "child_10"}
    childmap18 = {"LHC18d": "child_1", "LHC18e": "child_2", "LHC18f": "child_3",
                  "LHC18g": "child_4", "LHC18h": "child_5", "LHC18i": "child_6",
                  "LHC18j": "child_7", "LHC18k": "child_8", "LHC18l": "child_9",
                  "LHC18m": "child_10", "LHC18n": "child_11", "LHC18o": "child_12",
                  "LHC18p": "child_13"}
    childmaps = {"16": childmap16, "17": childmap17, "18": childmap18}
    year = sys.argv[1]
    trainrun = sys.argv[2]
    basedir = os.getcwd()
    for period, child in childmaps[year].iteritems():
        outdir = os.path.join(basedir, period)
        if not os.path.exists(outdir):
            os.makedirs(outdir, 0o755)
        outfile = os.path.join(outdir, "AnalysisResults.root")
        infile = os.path.join("/alice/cern.ch/user/a/alitrain", "{0}_{1}".format(trainrun, child), "merge", "AnalysisResults.root")
        command = "alien_cp alien://{0} {1}".format(infile, outfile)
        subprocess.call(command, shell=True)