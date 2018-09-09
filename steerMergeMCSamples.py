#! /usr/bin/env python
import os
import sys
import subprocess

if __name__ == "__main__":
    basedir = os.getcwd()
    repo = os.path.abspath(os.path.dirname(sys.argv[0]))
    samples = [x for x in os.listdir(basedir) if "LHC" in x]
    script = "mergeRuns.py"

    for s in samples:
        period = s.split("_")[0]
        os.chdir(s)
        command = "%s %s -r %s -m merged_calo -n 4" %(os.path.join(repo, script), os.path.join(basedir, s), period)
        print command
        subprocess.call(command, shell = True)
        os.chdir(basedir)
    
    # merge all periods
    periodmergescript = "mergeMCPtHardDatasets.py"
    command = "%s %s -m merged_calo -n 4" %(os.path.join(repo, periodmergescript), basedir)
    subprocess.call(command, shell = True)

    # merge simtrees
    simtreemergescript = "mergeallsimtrees.py"
    os.chdir("merged_calo")
    command = os.path.join(repo, simtreemergescript)
    subprocess.call(command, shell = True)
    os.chdir(basedir)
