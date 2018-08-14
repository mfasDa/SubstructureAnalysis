#! /usr/bin/env python

from __future__ import print_function
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
    JETTYPE = "FullJets"
    REPO = getrepo()
    SCRIPTS = ["unfoldJetPtSpectrumBayes.cpp", "unfoldJetPtSpectrumSvd.cpp"] 
    WORKQUEUE = workpool()
    for TRIGGER in ["INT7"]:
        for RADIUS in range(2, 6):
            FILEDATA = os.path.join("data", "merged_1617" if TRIGGER == "INT7" else "merged_17", "JetSubstructureTree_%s_R%02d_%s.root" %(JETTYPE, RADIUS, TRIGGER))
            FILEMC = os.path.join("mc", "merged", "JetSubstructureTree_%s_R%02d_INT7_merged.root" %(JETTYPE, RADIUS))
            print("Unfolding %s, R=%.1f" %(TRIGGER, float(RADIUS)))
            for SCRIPT in SCRIPTS:
                cmd="root -l -b -q \'%s(\"%s\", \"%s\")'" %(os.path.join(REPO, SCRIPT), FILEDATA, FILEMC)
                print("Command: %s" %cmd)
                WORKQUEUE.insert(cmd)

    WORKERS = []
    for IWORK in range(0, 10):
        WORKER = Processor(WORKQUEUE)
        WORKER.start()
        WORKERS.append(WORKER)
    for WORKER in WORKERS:
        WORKER.join()