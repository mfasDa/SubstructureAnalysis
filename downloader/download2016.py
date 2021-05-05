#! /usr/bin/env python3

import os
import sys
import subprocess

if __name__ == "__main__":
    trainrun = sys.argv[1]
    childs = {"LHC16h":4, "LHC16i":5, "LHC16j":6, "LHC16k":7, "LHC16l":8, "LHC16o":9, "LHC16p":10}
    for period,child in childs.items():
        inputdir = "/alice/cern.ch/user/a/alitrain/PWGJE/Jets_EMC_pp/{}_child_{}/merge".format(trainrun, child)
        outputdir = os.path.join(os.getcwd(), period)
        if not os.path.exists(outputdir):
            os.makedirs(outputdir, 0o755)
        subprocess.call("alien_cp {}/AnalysisResults.root file://{}/AnalysisResults.root".format(inputdir, outputdir), shell=True)
