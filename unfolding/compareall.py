#! /usr/bin/env python

from __future__ import print_function
import os
import subprocess
import sys

if __name__ == "__main__":
    OBSERVABLE = sys.argv[1]
    JETTYPE = "FullJets"
    SCRIPTDIR         = "/data1/markus/Fulljets/pp_13TeV/Substructuretree/code/unfolding"
    COMPUNFOLD        = "comparisons/makeComparisonUnfoldedRaw.cpp" 
    COMPFOLD          = "comparisons/makeComparisonFoldingRaw.cpp" 
    CLOSURETEST       = "comparisons/makeClosureTest.cpp"
    SELFCLOSURETEST   = "comparisons/makeSelfClosureTest.cpp"
    COMPFOLDPT        = "comparisons/makeComparisonFoldingRawPt.cpp" 
    CLOSURETESTPT     = "comparisons/makeClosureTestPt.cpp"
    SELFCLOSURETESTPT = "comparisons/makeSelfClosureTestPt.cpp"
    PEARSONOBSERVABLE = "pearson/makePlotPearson_%s.cpp" %OBSERVABLE
    PEARSONPT         = "pearson/makePlotPearson_pt.cpp"
    RESPONSEMATRIX    = "responsematrix/makePlotResponseMatrixProjection_%s.cpp" %OBSERVABLE
    SCRIPTS= [COMPUNFOLD, COMPFOLD, CLOSURETEST, SELFCLOSURETEST, COMPFOLDPT, CLOSURETESTPT, SELFCLOSURETESTPT, PEARSONOBSERVABLE, PEARSONPT, RESPONSEMATRIX]
    #SCRIPTS= [PEARSONOBSERVABLE]
    #SCRIPTS= [CLOSURETEST, SELFCLOSURETEST]
    #SCRIPTS= [COMPUNFOLD, COMPFOLDPT]
    for TRIGGER in ["INT7", "EJ1", "EJ2"]:
    #for TRIGGER in ["INT7", "EJ1"]:
        for RADIUS in range(2, 6):
            FILERESULT = "JetSubstructureTree_%s_R%02d_%s_unfolded_%s.root" %(JETTYPE, RADIUS, TRIGGER, OBSERVABLE)
            print("Comparing %s, R=%.1f" %(TRIGGER, float(RADIUS)))
            for SCRIPT in SCRIPTS:
                cmd="root -l -b -q \'%s(\"%s\")'" %(os.path.join(SCRIPTDIR, SCRIPT), FILERESULT)
                print("Command: %s" %cmd)
                subprocess.call(cmd, shell=True)

