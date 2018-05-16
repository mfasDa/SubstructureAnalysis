#! /usr/bin/env python
from __future__ import print_function

import os
import sys

def get_list_of_rootfiles(inputdir, rootfilename):
  filelist = []
  for root, dirs, files in os.walk(inputdir):
    for f in files:
      if rootfilename in f:
        filelist.append(os.path.join(root, f))
        print("added %s" %(os.path.join(root, f)))
  return filelist

def extract_trigger_correlations(inputdir, rootfile):
  print("input dir: %s" %inputdir)
  script = "%s/extractTriggerCorrelation.cpp" %os.path.dirname(sys.argv[0])

  for f in [r for r in get_list_of_rootfiles(inputdir, rootfile) if '000' in r]:
    print("Processing %s" %f)
    os.system("root -l -b -q \'%s(\"ClusterQA_Default\",\"%s\")\'" %(script, f))

if __name__ == "__main__":
  extract_trigger_correlations(sys.argv[1] if len(sys.argv) > 1 else os.getcwd(), "AnalysisResults.root")