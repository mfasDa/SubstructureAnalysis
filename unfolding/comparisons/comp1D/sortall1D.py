#! /usr/bin/env python

import os
import shutil
import sys

def find_files(inputdir, tag, trigger, format):
    matcher = lambda myf : tag in myf and trigger in myf and format in myf
    return [myf for myf in os.listdir(inputdir) if matcher(myf)]
    
def run_sort(inputdir):
    triggers = ["INT7"] #, "EJ1", "EJ2"]
    formats = ["eps", "pdf", "png", "jpg", "gif"]
    basedirs = ["MCClosureTestEnergyBayes", "MCClosureTestEnergySvd", "comparisonUnfoldedRawBayes",  "comparisonUnfoldedRawSvd", "comparisonFoldRawBayes", "comparisonFoldRawSvd", "comparisonIterationsEnergyBayes", "comparisonRegSvd", "ComparisonBayesSvd"]
    for myb in basedirs:
        for myt in triggers:
            for myform in formats:
                outdir = os.path.join(myb, myt, myform)
                if not os.path.exists(outdir):
                    os.makedirs(outdir, 0755)
                for matchfile in find_files(inputdir, myb, myt, myform):
                    infile = os.path.join(inputdir, matchfile)
                    outfile = os.path.join(outdir, matchfile)
                    shutil.move(infile, outfile)

if __name__ == "__main__":
    run_sort(os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.getcwd()))