#! /usr/bin/env python

from __future__ import print_function
import argparse
import os
import subprocess
import sys
from threading import Thread, Lock

class workpool:

    def __init__(self):
        self.__tasks = []
        self.__mutex = Lock()

    def insert(self, task):
        self.__mutex.acquire()
        self.__tasks.append(task)
        self.__mutex.release()

    def pop(self):
        payload = None
        self.__mutex.acquire()
        if len(self.__tasks):
            payload = self.__tasks.pop(0)
        self.__mutex.release()
        return payload

class Processor(Thread):
    
    def __init__(self, workqueue):
        Thread.__init__(self)
        self.__workqueue = workqueue

    def run(self):
        task = self.__workqueue.pop()
        while task != None:
            subprocess.call(task, shell = True)
            task = self.__workqueue.pop()

def getrepo(): 
    sciptname = os.path.abspath(sys.argv[0])
    return os.path.dirname(sciptname)
    
if __name__ == "__main__":
    REPO = getrepo()
    parser = argparse.ArgumentParser(prog="runCorrrectionEnregy.py", description="Run correction chain 1D")
    parser.add_argument("datadir", metavar="DATADIR", help="Location where to find the data")
    parser.add_argument("-z", "--zleading", type=float, default=1.1, help="Cut on the leading neutral constituent")
    args = parser.parse_args()
    SCRIPTS = ["runCorrectionChain1DBayes_SysFakeTrgSwap.cpp"] 
    #SCRIPTS = ["runCorrectionChain1DBayes_SysRegTrgSwap.cpp"] 
    DATADIR = args.datadir
    ZCUT= args.zleading
    #CUTOFFS = [50., 60., 70., 80., 90., 100., 120.]
    CUTOFFS = [120.]
    BASEDIR = os.getcwd()
    for CUT in CUTOFFS:
        cutoffdir = os.path.join(BASEDIR, "cutoff%d" %(int(CUT)))
        if not os.path.exists(cutoffdir):
            os.makedirs(cutoffdir, 0755)
        os.chdir(cutoffdir)

        WORKQUEUE = workpool()
        for RADIUS in range(2, 6):
            print("Unfolding R=%.1f" %(float(RADIUS)/10.))
            for SCRIPT in SCRIPTS:
                cmd="root -l -b -q \'%s(%f, %f, %f, \"%s\")'" %(os.path.join(REPO, SCRIPT), float(RADIUS)/10., ZCUT, CUT, DATADIR)
                print("Command: %s" %cmd)
                WORKQUEUE.insert(cmd)

        WORKERS = []
        for IWORK in range(0, 4):
            WORKER = Processor(WORKQUEUE)
            WORKER.start()
            WORKERS.append(WORKER)
        for WORKER in WORKERS:
            WORKER.join()

        os.chdir(BASEDIR)