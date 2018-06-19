#! /usr/bin/env python

import os
import subprocess
import sys
import threading

script=os.path.join(os.path.abspath(os.path.dirname(sys.argv[0])), "extractJetSpectrum.cpp")

class Pool:
  
  def __init__(self):
    self.__lock = threading.Lock()
    self.__data = []

  def getpoolsize(self):
    return len(self.__data)

  def insertpool(self, filename):
    self.__lock.acquire(True)
    self.__data.append(filename)
    self.__lock.release()

  def pop(self):
    result = None
    self.__lock.acquire(True)
    if self.getpoolsize():
      result = self.__data.pop(0) 
    self.__lock.release()
    return result

class Worker(threading.Thread):
  
  def __init__(self, pool):
    threading.Thread.__init__(self)
    self.__pool = pool

  def run(self):
    haswork = True
    while haswork:
      nextfile = self.__pool.pop()
      if nextfile:
        scriptcmd = "root -l -b -q \'%s(\"%s\")\'" %(script, nextfile)
        os.system(scriptcmd)
      else:
        haswork = False
        

if __name__ == "__main__":
  DATAPOOL = Pool()
  BASEDIR = sys.argv[1] if len(sys.argv) > 1 else os.getcwd()
  for RUN in [x for x in os.listdir(BASEDIR) if x.isdigit()]:
    RUNFILE = os.path.join(BASEDIR, RUN, "AnalysisResults.root")
    if os.path.exists(RUNFILE):
      DATAPOOL.insertpool(RUNFILE)
  
  WORKERS = []
  for WORK in range(0, 10):
    MYWORK = Worker(DATAPOOL)
    MYWORK.start()
    WORKERS.append(MYWORK)

  for WORK in WORKERS:
    WORK.join()
