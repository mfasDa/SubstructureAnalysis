#! /usr/bin/env python

from __future__ import print_function
import argparse
import logging
import os
import subprocess
import sys
import threading

def getrepo(): 
    sciptname = os.path.abspath(sys.argv[0])
    return os.path.dirname(sciptname)

class workqueue:

    def __init__(self):
        self.__tasks = []
        self.__lock = threading.Lock()

    def addtask(self, task):
        self.__lock.acquire()
        self.__tasks.append(task)
        self.__lock.release()

    def pop(self):
        task = None 
        self.__lock.acquire()
        if len(self.__tasks):
            task = self.__tasks.pop()
        self.__lock.release()
        return task

class taskrunner(threading.Thread):

    def __init__(self, workerID, workqueue):
        threading.Thread.__init__(self)
        self.__workerID = workerID
        self.__workquene = workqueue

    def run(self):
        nextitem = self.__workquene.pop()
        while nextitem:
            logging.info("Processing %s" %nextitem)
            subprocess.call(nextitem, shell=True)
        logging.info("Worker %d finished work", self.__workerID)
    
if __name__ == "__main__":
    logging.basicConfig(format='[%(levelname)s]: %(message)s', level=logging.INFO)
    parser = argparse.ArgumentParser(prog="unfoldallStruct.py", description="Unfold all radii and triggers for stucture")
    parser.add_argument("observable", metavar="OBSERVABLE", help = "Observable to be unfolded")
    parser.add_argument("-e", "--eventsel", nargs='*', required=False, help="Triggers to be unfolded")
    parser.add_argument("-t", "--tag", type=str, default="", help = "Tag of the unfolding macro")
    parser.add_argument("-n", "--nworkers", type=int, default=4, help="Number of parallel workers")
    args = parser.parse_args()
    OBSERVABLE = sys.argv[1]
    JETTYPE = "FullJets"
    SCRIPTNAME = "RunUnfolding%sV1" %args.observable
    if len(args.tag):
        SCRIPTNAME += "_%s" %args.tag
    SCRIPTNAME += ".cpp"
    logging.info("Using script name %s" %SCRIPTNAME)
    SCRIPT = os.path.join(getrepo(), SCRIPTNAME)
    QUEUE = workqueue()
    DEFAULTTRIGGERS = ["INT7", "EJ1", "EJ2"]
    TRIGGERS = []
    if args.eventsel and len(args.eventsel):
        TRIGGERS = args.eventsel
    else:
        TRIGGERS = DEFAULTTRIGGERS
    for TRIGGER in TRIGGERS:
        for RADIUS in range(2, 6):
            FILEDATA = os.path.join("data", "merged_1617" if TRIGGER == "INT7" else "merged_17", "JetSubstructureTree_%s_R%02d_%s.root" %(JETTYPE, RADIUS, TRIGGER))
            FILEMC = os.path.join("mc", "merged_barrel", "JetSubstructureTree_%s_R%02d_INT7_merged.root" %(JETTYPE, RADIUS))
            logging.info("Unfolding %s, R=%.1f" %(TRIGGER, float(RADIUS)))
            cmd="root -l -b -q \'%s(\"%s\", \"%s\")'" %(SCRIPT, FILEDATA, FILEMC)
            logging.info("Command: %s" %cmd)
            QUEUE.addtask(cmd)

    WORKERS = []
    for IWORK in range(0, args.nworkers+1):
        WORKER = taskrunner(IWORK, QUEUE)
        WORKER.start()
        WORKERS.append(WORKER)
    for WORKER in WORKERS:
        WORKER.join()

    