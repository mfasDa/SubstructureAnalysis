#! /usr/bin/env python
from __future__ import print_function
import os, sys

def ExecMerge(outputfile, filelist):
    print("Merging output to %s" %outputfile)
    mergecommand = "hadd -f %s" %(outputfile)
    for gridfile in filelist:
        print("Adding %s" %gridfile)
        mergecommand += " %s" %(gridfile)
    os.system(mergecommand)

def GetFilelist(inputpath, filename):
    print("walking %s" %inputpath)
    result = []
    for root, dirs, files in os.walk(inputpath):
        for f in files:
            if filename in f:
                result.append(os.path.join(root, f))
    return result

def DoMerge(inputpath, filename):
    mergedir = "%s/merged" %(inputpath)
    if not os.path.exists(mergedir):
        os.makedirs(mergedir, 0755)

    for pthard in sorted(os.listdir(inputpath)):
        if not pthard.isdigit():
            continue
        print("Merging all file from pt-hard bin %s" %(pthard))
        outputdir = os.path.join(mergedir, pthard)
        if not os.path.exists(outputdir):
            os.makedirs(outputdir, 0755)
        ExecMerge(os.path.join(outputdir, filename), GetFilelist(os.path.join(inputpath, pthard), filename))
    print "Done"

if __name__ == "__main__":
    inputpath = sys.argv[1]
    rootfile = "AnalysisResults.root"
    if len(sys.argv) > 2:
        rootfile = sys.argv[2]
    DoMerge(inputpath, rootfile)
