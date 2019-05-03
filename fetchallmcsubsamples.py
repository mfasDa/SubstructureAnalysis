#! /usr/bin/env python3
import os
import subprocess
import sys

def GetYear(sample):
    return 2000 + int(sample[3:4])

if __name__ == "__main__":
    metamaps = {"LHC17f8": {"LHC16h": "LHC17f8j", "LHC16i": "LHC17f8g", 
                            "LHC16j": "LHC17f8d", "LHC16k": "LHC17f8f", 
                            "LHC16l": "LHC16f8a", "LHC16o": "LHC17f8e", 
                            "LHC16p": "LHC17f8k"},
                "LHC18f5": {"none1": "LHC18f5_1", "none2": "LHC18f5_2"}}
    repo = os.path.abspath(os.path.dirname(sys.argv[0]))
    trainrun = sys.argv[1]
    dataset = sys.argv[2]
    script = "copyFromGrid.py"
    basedir = os.getcwd()
    for period, sample in metamaps[dataset].items():
        outtag = sample if "none" in period else "{0}_{1}".format(period, sample)
        outdir = os.path.join(basedir, outtag)
        if not os.path.exists(outdir):
            os.makedirs(outdir, 0o755)
        os.chdir(outdir)
        proddir = os.path.join("/alice/sim", GetYear(dataset), sample)
        command= "{0} {1} {2} {3} -f AnalysisResults.root -n 4".format(os.path.join(repo, script), proddir, trainrun, outdir)
        subprocess.call(command, shell=True)
