#! /usr/bin/env python

import os
import sys

def merge(outputfile, filestomerge):
    cmd = "hadd -f %s" %outputfile
    for f in filestomerge:
        cmd += " %s" %f
    os.system(cmd)

def extract_run(path):
    result = -1
    tokens = path.split("/")
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
    return [f for f in allfiles if extract_run(f) in runlist]

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
    if not os.path.exists(outputdir):
        os.makedirs(outputdir)
    merge(outputfile, find_files(inputdir, rootfilename, parse_runlist(runlist)))
    
if __name__ == "__main__":
    merge_runs_filtered(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])