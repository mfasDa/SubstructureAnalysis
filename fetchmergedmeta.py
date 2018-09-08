#! /usr/bin/env python
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
    childmaps = {"16": childmap16, "17": childmap17}
    year = sys.argv[1]
    trainrun = sys.argv[2]
    basedir = os.getcwd()
    for period, child in childmaps[year].iteritems():
        outdir = os.path.join(basedir, period)
        if not os.path.exists(outdir):
            os.makedirs(outdir, 0755)
        outfile = os.path.join(outdir, "AnalysisResults.root")
        infile = os.path.join("/alice/cern.ch/user/a/alitrain", "%s_%s" %(trainrun, child), "merge", "AnalysisResults.root")
        command = "alien_cp alien://%s %s" %(infile, outfile)
        subprocess.call(command, shell=True)