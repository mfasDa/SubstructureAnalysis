#! /usr/bin/env python

from __future__ import print_function
import os
import subprocess

if __name__ == "__main__":
    SCRIPTDIR         = "/alf/data/mfasel/Fulljets/pp_13TeV/Substructuretree/code/unfolding/comparisons/comp1D"
    COMPUNFOLD        = "ComparisonUnfoldedRaw_SpectrumTask.cpp"
    COMPFOLD          = "ComparisonFoldRaw_SpectrumTask.cpp"
    CLOSURETEST       = "MCClosureTest1D_SpectrumTask.cpp"
    # special scripts
    #COMPITERBAYES     = "ComparisonIterations1D.cpp"
    COMPREG        = "ComparisonRegularization_SpectrumTask.cpp"
    COMPBAYESSVD      = "ComparisonBayesSVD_SpectrumTask.cpp"
    SCRIPTS= [COMPUNFOLD, COMPFOLD, CLOSURETEST]
    #SCRIPTS= [COMPUNFOLD, COMPFOLDPT]
    for RADIUS in range(2, 6):
        FILERESULT_SVD = "corrected1DSVD_R%02d.root" %(RADIUS)
        FILERESULT_Bayes = "corrected1DBayes_R%02d.root" %(RADIUS)
        print("Comparing R=%.1f" %(float(RADIUS)))
        # first execute special scripts
        if os.path.exists(FILERESULT_Bayes):
            cmd = "root -l -b -q \'%s(\"%s\")'" %(os.path.join(SCRIPTDIR, COMPITERBAYES), FILERESULT_Bayes)
            print("Command: %s" %cmd)
            subprocess.call(cmd, shell=True)
        if os.path.exists(FILERESULT_SVD):
            cmd = "root -l -b -q \'%s(\"%s\")'" %(os.path.join(SCRIPTDIR, COMPREG), FILERESULT_SVD)
            print("Command: %s" %cmd)
            subprocess.call(cmd, shell=True)
        if os.path.exists(FILERESULT_Bayes) and os.path.exists(FILERESULT_SVD):
            cmd = "root -l -b -q \'%s(\"%s\", \"%s\")'" %(os.path.join(SCRIPTDIR, COMPBAYESSVD), FILERESULT_Bayes, FILERESULT_SVD)
            print("Command: %s" %cmd)
            subprocess.call(cmd, shell=True)
        for SCRIPT in SCRIPTS:
            for UNFOLDED in [FILERESULT_Bayes, FILERESULT_SVD]:
                if os.path.exists(UNFOLDED):
                    cmd="root -l -b -q \'%s(\"%s\")'" %(os.path.join(SCRIPTDIR, SCRIPT), UNFOLDED)
                    print("Command: %s" %cmd)
                    subprocess.call(cmd, shell=True)
