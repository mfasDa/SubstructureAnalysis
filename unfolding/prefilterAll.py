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
    SCRIPT = os.path.join(getrepo(), "prefilterMC", "prefilter%s.cpp" %OBSERVABLE)
    for TRIGGER in ["INT7", "EJ1", "EJ2"]:
        for RADIUS in range(2, 6):
            FILEMC = "JetSubstructureTree_%s_R%02d_%s_merged.root" %(JETTYPE, RADIUS, TRIGGER)
            cmd="root -l -b -q \'%s(\"%s\")\'" %(SCRIPT, FILEMC)
            print("Command: %s" %cmd)
            subprocess.call(cmd, shell=True)