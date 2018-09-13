#! /usr/bin/env python

from __future__ import print_function
import argparse
import os
import subprocess

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog = "compareall.py", description = "Make all comparison plots")
    parser.add_argument("observable", metavar="OBSERVABLE", type=str, help="Name of the observable")
    parser.add_argument("-t", "--triggers", nargs="*", required=False, help="Run macro only for certain triggers")
    args = parser.parse_args()
    OBSERVABLE = args.observable
    JETTYPE = "FullJets"
    SCRIPTDIR         = "/data1/markus/Fulljets/pp_13TeV/Substructuretree/code/unfolding"
    COMPUNFOLD        = "comparisons/makeComparisonUnfoldedRaw.cpp" 
    COMPFOLD          = "comparisons/makeComparisonFoldingRaw.cpp" 
    CLOSURETEST       = "comparisons/makeClosureTest.cpp"
    SELFCLOSURETEST   = "comparisons/makeSelfClosureTest.cpp"
    COMPFOLDPT        = "comparisons/makeComparisonFoldingRawPt.cpp" 
    CLOSURETESTPT     = "comparisons/makeClosureTestPt.cpp"
    SELFCLOSURETESTPT = "comparisons/makeSelfClosureTestPt.cpp"
    EFFKINE           = "comparisons/makePlotEffKine.cpp"
    PEARSONOBSERVABLE = "pearson/makePlotPearson_%s.cpp" %OBSERVABLE
    PEARSONPT         = "pearson/makePlotPearson_pt.cpp"
    STATS             = "stats/plotStatsFromUnfoldingOutput.cpp"
    RESPONSEMATRIX    = "responsematrix/makePlotResponseMatrixProjection_%s.cpp" %OBSERVABLE
    SCRIPTS= [COMPUNFOLD, COMPFOLD, CLOSURETEST, SELFCLOSURETEST, COMPFOLDPT, CLOSURETESTPT, SELFCLOSURETESTPT,
              PEARSONOBSERVABLE, PEARSONPT, RESPONSEMATRIX, STATS, EFFKINE]
    #SCRIPTS= [PEARSONOBSERVABLE]
    #SCRIPTS= [CLOSURETEST, SELFCLOSURETEST]
    #SCRIPTS= [COMPUNFOLD, COMPFOLDPT]
    #SCRIPTS=[EFFKINE]
    defaulttriggers = ["INT7", "EJ1", "EJ2"]
    TRIGGERS=[]
    if args.triggers and len(args.triggers):
        TRIGGERS = args.triggers
    else:
        TRIGGERS = defaulttriggers
    for TRIGGER in TRIGGERS:
    #for TRIGGER in ["INT7", "EJ1"]:
        for RADIUS in range(2, 6):
            FILERESULT = "JetSubstructureTree_%s_R%02d_%s_unfolded_%s.root" %(JETTYPE, RADIUS, TRIGGER, OBSERVABLE)
            print("Comparing %s, R=%.1f" %(TRIGGER, float(RADIUS)))
            for SCRIPT in SCRIPTS:
                cmd="root -l -b -q \'%s(\"%s\")'" %(os.path.join(SCRIPTDIR, SCRIPT), FILERESULT)
                print("Command: %s" %cmd)
                subprocess.call(cmd, shell=True)

