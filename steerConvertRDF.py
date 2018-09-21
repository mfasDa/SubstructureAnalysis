#! /usr/bin/env python
import os
import subprocess
import sys

if __name__ == "__main__":
    SCRIPT=os.path.join(os.path.abspath(os.path.dirname(sys.argv[0])), "convertRDF.cpp")
    TRIGGERS=["INT7", "EJ1", "EJ2"]
    for TRG in TRIGGERS:
        for R in range(2, 6):
            treename = "JetSubstructureTree_FullJets_R%02d_%s" %(R, TRG)
            for PT in range(1, 21):
                FILENAME = os.path.join("%02d" %PT, "AnalysisResults.root")
                CMD = "root -l -b -q \'%s(\"%s\", \"%s\", %d)\'" %(SCRIPT, FILENAME, treename, PT)
                print "Running %s" %CMD
                subprocess.call(CMD, shell=True)