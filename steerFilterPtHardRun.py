#! /usr/bin/env python

import os
import sys

def find_files(inputdir, rootfilename):
  files = []
  for r, d, f in os.walk(inputdir):
    for l in f:
      if os.path.basename(l) == rootfilename:
        files.append(os.path.join(r,l))
  return sorted(files)

def main(inputdir, rootfile):
  script=os.path.join(os.path.abspath(os.path.dirname(sys.argv[0])), "FilterTree.cpp")
  base = os.getcwd()
  print "IAMHERE"
  for f in find_files(inputdir, rootfile):
    os.chdir(os.path.dirname(f))
    print "Processing %s" %f
    os.system("root -l -b -q \'%s(\"%s\")\'" %(script, f))
    os.chdir(base)

if __name__ == "__main__":
    print "IMAHERE"
    main(os.getcwd(), sys.argv[1])
