#! /usr/bin/env python

import logging
import os
import sys

script = os.path.join(os.path.dirname(os.path.abspath(sys.argv[0])), "extractClusterSpectra.cpp")

def getlistoffiles(inputdir, filetofind):
  logging.info("searching %s in inputdir %s, ", filetofind, inputdir)
  listoffiles = []
  for root, dirs, files in os.walk(inputdir):
    for f in files:
      if os.path.basename(f) == filetofind:
        listoffiles.append(os.path.join(root, f))
  return listoffiles

def extractrunclusterspectrum(basefile, tag):
  logging.info("Processing %s", basefile)
  currentdir = os.getcwd()
  rundir = os.path.dirname(basefile)
  runfile = os.path.basename(basefile)
  os.chdir(rundir)
  os.system("root -l -b -q \'%s(\"%s\", \"%s\")\'" %(script, tag, runfile))
  os.chdir(currentdir)

def extractclusterspectra(inputdir, rootfile, tag):
  for specfile in getlistoffiles(inputdir, rootfile):
    extractrunclusterspectrum(specfile, tag)

if __name__ == "__main__":
  logging.basicConfig(format='[%(levelname)s]: %(message)s', level=logging.INFO)
  extractclusterspectra(sys.argv[2] if len(sys.argv) > 2 else os.getcwd(), sys.argv[1], sys.argv[3] if len(sys.argv) > 3 else "INT7")