#! /usr/bin/env python
import os
import sys

def find_files(inputdir, rootfilename):
  files = []
  for r, d, f in os.walk(inputdir):
    for l in f:
      if os.path.basename(l) == rootfilename:
        files.append(os.path.join(r,l))
  return sorted(files);

def main(inputdir, rootfilename):
  script = os.path.join(os.path.dirname(os.path.abspath(sys.argv[0])), "reweight.cpp")
  basedir = os.getcwd()
  for f in find_files(inputdir, rootfilename):
    os.chdir(os.path.dirname(f))
    print "Reweighting %s" %f
    os.system("root -l -b -q \'%s(\"%s\")\'" %(script, f))

if __name__ == "__main__":
  main(os.getcwd(), "AnalysisResults.root" if len(sys.argv) < 2 else sys.argv[1])