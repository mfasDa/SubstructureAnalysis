#! /usr/bin/env python3

import os
import sys
import subprocess

if __name__ == "__main__":
    repo = os.path.dirname(os.path.abspath(sys.argv[0]))
    executable = os.path.join(repo, "runMergePair.sh")
    inputdir = sys.argv[1]
    outputdir = sys.argv[2]

    file1=""
    file2=""
    nfile=0
    nslot=0
    for fl in os.listdir(inputdir):
        filename = os.path.join(inputdir, fl, "AnalysisResults.root")
        if nfile == 0:
            file1 = filename
            nfile += 1
        else:
            file2 = filename
            outputfile = os.path.join(outputdir, "AnalysisResults_%d.root" %nslot)
            logfile = os.path.join(outputdir, "merge_%d.log" %nslot)
            jobname = "merge%d" %nslot
            subprocess.call("sbatch -N1 -n1 --partition=short -J %s -o %s %s %s %s %s" %(jobname, logfile, executable, file1, file2, outputfile), shell = True)
            file1 = ""
            file2 = ""
            nfile = 0
            nslot += 1