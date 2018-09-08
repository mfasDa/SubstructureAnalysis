#! /usr/bin/env python
import os
import subprocess
import sys

if __name__ == "__main__":
    metamap = {"LHC16h": "LHC17f8j", "LHC16i": "LHC17f8g", 
               "LHC16j": "LHC17f8d", "LHC16k": "LHC17f8f", 
               "LHC16l": "LHC16f8a", "LHC16o": "LHC17f8e", 
               "LHC16p": "LHC17f8k"}
    repo = os.path.abspath(os.path.dirname(sys.argv[0]))
    trainrun = sys.argv[1]
    script = "copyFromGrid.py"
    basedir = os.getcwd()
    for period, sample in metamap.iteritems():
        outtag = "%s_%s" %(period, sample)
        outdir = os.path.join(basedir, outtag)
        if not os.path.exists(outdir):
            os.makedirs(outdir, 0755)
        os.chdir(outdir)
        proddir = "/alice/sim/2017/" + sample
        command= "%s %s %s %s -f AnalysisResults.root -n 4" %(os.path.join(repo, script), proddir, trainrun, outdir)
        subprocess.call(command, shell=True)
