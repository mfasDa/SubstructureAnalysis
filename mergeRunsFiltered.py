#! /usr/bin/env python

from __future__ import print_function
import os
import sys

def merge(outputfile, filestomerge):
    cmd = "hadd -f %s" %outputfile
    for f in filestomerge:
        cmd += " %s" %f
    os.system(cmd)

def extract_run(path, inputdir):
    mypath = path.replace(inputdir, "")
    result = -1
    tokens = mypath.split("/")
    for t in tokens:
        if t.isdigit():
            result = int(t)
            break
    return result

def find_files(inputdir, rootfile, runlist):
    allfiles = []
    for root, dirs, files in os.walk(inputdir):
        for f in files:
            if rootfile in f:
                allfiles.append(os.path.join(root, f))
    return [f for f in allfiles if extract_run(f, inputdir) in runlist]

def parse_runlist(runlistname):
    runs = []
    reader = open(runlistname, 'r')
    for line in reader:
        content = line.split(",")
        for c in content:
            tmp = c.lstrip().rstrip()
            if not len(c):
                continue
            if(tmp.isdigit()):
                runs.append(int(tmp))
    reader.close()
    return runs

def merge_runs_filtered(outputfile, inputdir, rootfilename, runlist):
    outputdir = os.path.dirname(outputfile)
    if not outputdir or not len(outputdir):
        outputdir = os.getcwd()
    if not os.path.exists(outputdir):
        os.makedirs(outputdir)
    merge(outputfile, find_files(inputdir, rootfilename, parse_runlist(runlist)))

def Usage():
    print("Usage: ./merge_runs_filtered.py OUTPUTFILE INTPUTDIR ROOTFILENAME RUNLIST")
    print("")
    print("  OUTPUTFILE:     Full path of the output file")
    print("  INPUTDIR:       Directory in which to find files to merge")
    print("  ROOTFILENAME:   Name of the root file to merge")
    print("  RUNLIST:        List of good runs to be merged")
    
if __name__ == "__main__":
    if len(sys.argv) < 5:
        Usage()
        sys.exit(1)
    merge_runs_filtered(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])