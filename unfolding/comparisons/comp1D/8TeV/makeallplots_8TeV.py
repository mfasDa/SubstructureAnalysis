#! /usr/bin/env python

from __future__ import print_function
import os
import subprocess
import argparse

if __name__ == "__main__":
    # Get unfolded root files from the command line
    parser = argparse.ArgumentParser(prog = "makeallplots_8TeV.py", description = "Run 1D unfolding comparison macros for the jet spectrum task")
    parser.add_argument("svdfile",metavar = "SVD_UNFOLDING_RESULT", help="Example: Output file from SVD unfolding")
    parser.add_argument("bayesfile",metavar = "BAYESIAN_UNFOLDING_RESULT", help="Example: Output file from Bayesian unfolding")
    parser.add_argument("out",metavar = "OUTPUT_DIRECTORY", help="Example: Path to output directory for comparison plots")
    parser.add_argument("ftype",metavar = "FILE_TYPE", default="png", help="Example: File type: png, eps, pdf, etc")
    args = parser.parse_args()

    FILERESULT_SVD = args.svdfile
    FILERESULT_Bayes = args.bayesfile
    outputdir = args.out
    filetype = args.ftype

    # Unfolding comparisons directory
    SCRIPTDIR         = "/home/austin/alice/SubstructureAnalysis/unfolding/comparisons/comp1D/8TeV"

    # Special scripts
    COMPBAYESSVD      = "ComparisonBayesSVD_8TeV.cpp"

    # Main scripts
    COMPFOLD          = "ComparisonFoldRaw_8TeV.cpp"
    CLOSURETEST       = "MCClosureTest1D_8TeV.cpp"
    COMPREG           = "ComparisonRegularization_8TeV.cpp"
    SCRIPTS           = [COMPFOLD, CLOSURETEST, COMPREG]


    # Execute special scripts
    if os.path.exists(FILERESULT_Bayes) and os.path.exists(FILERESULT_SVD):
        cmd = "root -x -q -l -b \'%s(\"%s\", \"%s\", \"%s\", \"%s\")'" %(os.path.join(SCRIPTDIR, COMPBAYESSVD), FILERESULT_SVD, FILERESULT_Bayes, outputdir, filetype)
        print("Command: %s" %cmd)
        subprocess.call(cmd, shell=True)

    # Execute main scripts
    for SCRIPT in SCRIPTS:
        for UNFOLDED in [FILERESULT_Bayes, FILERESULT_SVD]:
            if os.path.exists(UNFOLDED):
                cmd="root -x -q -l -b \'%s(\"%s\", \"%s\", \"%s\")'" %(os.path.join(SCRIPTDIR, SCRIPT), UNFOLDED, outputdir, filetype)
                print("Command: %s" %cmd)
                subprocess.call(cmd, shell=True)
