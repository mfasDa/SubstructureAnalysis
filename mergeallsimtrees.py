#! /usr/bin/env python

from __future__ import print_function
import os
import subprocess
import sys
import threading

repo = os.path.dirname(os.path.abspath(sys.argv[0]))

class worker(threading.Thread):

	def __init__(self, treename):
		threading.Thread.__init__(self)
		self.__treename = treename

	def run(self):
		script = "%s/MergeResponseppNewV1.cpp" %repo
		os.system("root -l -b -q \'%s(\"%s\", \"%s\")\'" %(script, os.getcwd(), self.__treename))

if __name__ == "__main__":
  workers = []
  for trigger in ["INT7", "EJ1", "EJ2"]:
    for radius in range(2,6):
      print("Merging %s,R=%.1f" %( trigger, float(radius)/10.))
      treename = "JetSubstructureTree_FullJets_R%02d_%s" %(radius, trigger)
      print("Merging %s" %treename)
      merge  = worker(treename)
      merge.start()
      workers.append(merge)

  for w in workers:
    w.join()
