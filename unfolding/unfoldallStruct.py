#! /usr/bin/env python

from __future__ import print_function
import os
import subprocess
import sys

def getrepo(): 
    sciptname = os.path.abspath(sys.argv[0])
    return os.path.dirname(sciptname)
    
if __name__ == "__main__":
    OBSERVABLE = sys.argv[1]
    JETTYPE = "FullJets"
    SCRIPT = os.path.join(getrepo(), "RunUnfolding%sV1.cpp" %OBSERVABLE)
    for TRIGGER in ["INT7", "EJ1", "EJ2"]:
        for RADIUS in range(2, 6):
            FILEDATA = os.path.join("data", "merged_1617" if TRIGGER == "INT7" else "merged_17", "JetSubstructureTree_%s_R%02d_%s.root" %(JETTYPE, RADIUS, TRIGGER))
            FILEMC = os.path.join("mc", "merged", "JetSubstructureTree_%s_R%02d_INT7_merged.root" %(JETTYPE, RADIUS))
            print("Unfolding %s, R=%.1f" %(TRIGGER, float(RADIUS)))
            cmd="root -l -b -q \'%s(\"%s\", \"%s\")'" %(SCRIPT, FILEDATA, FILEMC)
            print("Command: %s" %cmd)
            subprocess.call(cmd, shell=True)