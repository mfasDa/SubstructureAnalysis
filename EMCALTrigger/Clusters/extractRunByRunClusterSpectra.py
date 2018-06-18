#! /usr/bin/env python

import logging
import os
import subprocess
import sys
import threading

script = os.path.join(os.path.dirname(os.path.abspath(sys.argv[0])), "extractClusterSpectra.cpp")

def extractrunclusterspectrum(basefile, tag):
  logging.info("Processing %s", basefile)
  currentdir = os.getcwd()
  rundir = os.path.dirname(basefile)
  runfile = os.path.basename(basefile)
  os.chdir(rundir)
  subprocess.call(['root', '-l', '-b', '-q', "\'%s(\"%s\", \"%s\")\'" %(script, tag, runfile)])
  os.chdir(currentdir)

class Pool :
  
  def __init__(self, data):
    self.__lock = threading.Lock()
    self.__data = data

  def getsize(self):
    return len(self.__data)

  def pop(self):
    self.__lock.acquire(True)
    procfile = self.__data.pop(0)
    self.__lock.release()
    return procfile

class Worker(threading.Thread):
  
  def __init__(self, threadid, pool, tag):
    threading.Thread.__init__(self)
    self.__id = threadid
    self.__pool = pool
    self.__tag = tag

  def getid(self):
    return self.__id

  def run(self):
    hasWork = True
    while hasWork:
      specfile = self.__pool.pop()
      if specfile:
        extractrunclusterspectrum(specfile, self.__tag)
      else:
        hasWork = False

def getlistoffiles(inputdir, filetofind):
  logging.info("searching %s in inputdir %s, ", filetofind, inputdir)
  listoffiles = []
  for root, dirs, files in os.walk(inputdir):
    for f in files:
      if os.path.basename(f) == filetofind:
        listoffiles.append(os.path.join(root, f))
  return listoffiles

def extractclusterspectra(inputdir, rootfile, tag):
  pool = Pool(getlistoffiles(inputdir, rootfile))

  workers = []
  for worker in range(0, 10):
    worker = Worker(worker, pool, tag)
    workers.append(worker)
    worker.start()

  for worker in workers:
    worker.join()

if __name__ == "__main__":
  logging.basicConfig(format='[%(levelname)s]: %(message)s', level=logging.INFO)
  extractclusterspectra(sys.argv[2] if len(sys.argv) > 2 else os.getcwd(), sys.argv[1], sys.argv[3] if len(sys.argv) > 3 else "INT7")