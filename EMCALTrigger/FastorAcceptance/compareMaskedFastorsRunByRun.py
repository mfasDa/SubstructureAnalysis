#! /usr/bin/env python

import logging
import os
import sys

scriptdir = os.path.abspath(os.path.dirname(sys.argv[0]))
rootmacro = os.path.join(scriptdir, "visualizeMaskComparisonRun.cpp")

def get_runs(basedir):
  return sorted([r for r in os.listdir(basedir) if r.isdigit()])

def find_file(filelist, pattern):
  return [f for f in filelist if pattern in f]

def do_comparison(basedir):
  base = os.getcwd()
  for r in get_runs(basedir):
    rundir = os.path.join(base, r)
    files = os.listdir(rundir)
    hasl0 = len(find_file(files, "maskedFastorsFreq_L0"))
    hasl1 = len(find_file(files, "maskedFastorsFreq_L1"))
    if not hasl0 and not hasl1:
      logging.warning("Skipping run %d because not both fastor lists were found ..." %int(r))
      continue
    os.chdir(rundir)
    logging.info("Visualizing dead FastORs or run %d ..." %int(r))
    os.system("root -l -b -q \'%s(%d)\'" %(rootmacro, int(r)))
    os.chdir(base)

if __name__ == "__main__":
  LOGLEVEL = logging.INFO
  logging.basicConfig(format='[%(levelname)s]: %(message)s', level=LOGLEVEL)
  do_comparison(sys.argv[1] if len(sys.argv) > 1 else os.getcwd())