#! /usr/bin/env python

from __future__ import print_function
import os
import subprocess
import sys


if __name__ == "__main__":
    OBSERVABLE = sys.argv[1]
    SCRIPTDIR = "/data1/markus/Fulljets/pp_13TeV/Substructuretree/code/unfolding/convergence"
    JETTYPE = "FullJets"
    BINSCRIPT = "Convergence_%s.cpp" %OBSERVABLE
    SUMSCRIPT = "ConvergenceSumPt_%s.cpp" %OBSERVABLE
    SCRIPTS = [BINSCRIPT, SUMSCRIPT]
    for TRIGGER in ["INT7", "EJ1", "EJ2"]:
        for RADIUS in range(2, 6):
            FILERESULT = "JetSubstructureTree_%s_R%02d_%s_unfolded_%s.root" %(JETTYPE, RADIUS, TRIGGER, OBSERVABLE)
            print("Comparing %s, R=%.1f" %(TRIGGER, float(RADIUS)))
            for SCRIPT in SCRIPTS:
                cmd="root -l -b -q \'%s(\"%s\")'" %(os.path.join(SCRIPTDIR, SCRIPT), FILERESULT)
                print("Command: %s" %cmd)
                subprocess.call(cmd, shell=True)