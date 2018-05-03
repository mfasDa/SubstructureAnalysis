#! /usr/bin/env python

import logging
import os
import sys

scriptdir = os.path.abspath(os.path.dirname(sys.argv[0]))
rootmacro = os.path.join(scriptdir, "extractDeadFastORsFreq.cpp")

def get_runs(basedir):
  return sorted([r for r in os.listdir(basedir) if r.isdigit()])

def extract_fastors(basedir):
  base = os.getcwd()
  for r in get_runs(basedir):
    rundir = os.path.join(base, r)
    if not os.path.exists(os.path.join(rundir, "AnalysisResults.root")):
      logging.warning("Skipping run %d because no AnalysisResults.root was found ..." %int(r))
      continue
    logging.info("Extracting dead FastORs or run %d ..." %int(r))
    os.chdir(rundir)
    for level in range(0, 2):
      os.system("root -l -b -q \'%s(%d, \"EG1\")\'" %(rootmacro, level))
    os.chdir(base)

if __name__ == "__main__":
  LOGLEVEL = logging.INFO
  logging.basicConfig(format='[%(levelname)s]: %(message)s', level=LOGLEVEL)
  extract_fastors(sys.argv[1] if len(sys.argv) > 1 else os.getcwd())