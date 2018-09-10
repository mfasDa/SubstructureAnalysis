#! /usr/bin/env python

from __future__ import print_function
import os
import subprocess
import sys

if __name__ == "__main__":
    SCRIPTDIR         = "/data1/markus/Fulljets/pp_13TeV/Substructuretree/code/unfolding/comparisons/comp1D"
    COMPUNFOLD        = "ComparisonUnfoldedRaw.cpp" 
    COMPFOLD          = "ComparisonFoldRaw.cpp" 
    CLOSURETEST       = "MCClosureTest1D.cpp"
    # special scripts
    COMPITERBAYES     = "ComparisonIterations1D.cpp" 
    COMPREGSVD        = "ComparisonRegularizationSVD.cpp"
    COMPBAYESSVD      = "ComparisonBayesSVD.cpp"
    SCRIPTS= [COMPUNFOLD, COMPFOLD, CLOSURETEST]
    #SCRIPTS= [COMPUNFOLD, COMPFOLDPT]
    for RADIUS in range(2, 6):
        FILERESULT_SVD = "corrected1DSVD_R%02d.root" %(RADIUS)
        FILERESULT_Bayes = "corrected1DBayes_R%02d.root" %(RADIUS)
        print("Comparing R=%.1f" %(float(RADIUS)))
        # first execute special scripts
        cmd = "root -l -b -q \'%s(\"%s\")'" %(os.path.join(SCRIPTDIR, COMPITERBAYES), FILERESULT_Bayes)
        print("Command: %s" %cmd)
        subprocess.call(cmd, shell=True)
        cmd = "root -l -b -q \'%s(\"%s\")'" %(os.path.join(SCRIPTDIR, COMPREGSVD), FILERESULT_SVD)
        print("Command: %s" %cmd)
        subprocess.call(cmd, shell=True)
        cmd = "root -l -b -q \'%s(\"%s\", \"%s\")'" %(os.path.join(SCRIPTDIR, COMPBAYESSVD), FILERESULT_Bayes, FILERESULT_SVD)
        print("Command: %s" %cmd)
        subprocess.call(cmd, shell=True)
        for SCRIPT in SCRIPTS:
            for UNFOLDED in [FILERESULT_Bayes, FILERESULT_SVD]:
                cmd="root -l -b -q \'%s(\"%s\")'" %(os.path.join(SCRIPTDIR, SCRIPT), UNFOLDED)
                print("Command: %s" %cmd)
                subprocess.call(cmd, shell=True)
