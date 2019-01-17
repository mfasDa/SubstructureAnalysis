#! /usr/bin/env python

import os
import subprocess
import sys

if __name__ == "__main__":
    SCRIPT=os.path.join(os.path.abspath(os.path.dirname(sys.argv[0])), "splitFile.C")
    base=os.getcwd()
    for d in os.listdir(base):
        if os.path.isdir(d):
            os.chdir(os.path.join(base, d))
            if os.path.exists("AnalysisResults.root"):
                subprocess.call("root -l -b -q %s" %SCRIPT, shell=True)
            os.chdir(base)