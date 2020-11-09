#! /usr/bin/env python3

import os
import subprocess

def run_merge(workdir, minpthard):
    cmd = "hadd -f AnalysisResults_pthard{PTHARD}.root".format(minpthard)
    for pthardbin in range(minpthard, 21):
        cmd += " %02d/AnalysisResults.root" %pthardbin
    subprocess.call(cmd, shell=True)
    pass

if __name__ == "__main__":
    workdir = os.getcwd()
    for minpthard in range(1, 6):
        run_merge(workdir, minpthard)