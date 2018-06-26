#! /usr/bin/env python
from __future__ import print_function
import logging
import os 
import threading
import sys

def ExecMerge(outputfile, filelist):
    logging.info("Merging output to %s" %outputfile)
    mergecommand = "hadd -f %s" %(outputfile)
    for gridfile in filelist:
        logging.info("Adding %s" %gridfile)
        mergecommand += " %s" %(gridfile)
    os.system(mergecommand)

def GetFilelist(inputpath, filename):
    logging.debug("walking %s" %inputpath)
    result = []
    for root, dirs, files in os.walk(inputpath):
        for f in files:
            if filename in f:
                result.append(os.path.join(root, f))
    return result

class Workqueue:

    class __queueentry:
        
        def __init__(self, outputfile, filelist):
            self.__outputfile = outputfile
            self.__filelist = filelist

        def setoutputfile(self, outputfile):
            self.__outputfile = outputfile

        def getoutputfile(self):
            return self.__outputfile

        def setfilelist(self, filelist):
            self.__filelist = filelist

        def getfilelist(self):
            return self.__filelist

        Outputfile = property(getoutputfile, setoutputfile)
        Filelist = property(getfilelist, setfilelist)
    
    def __init__(self):
        self.__data = []
        self.__lock = threading.Lock()

    def push_back(self, outputfile, filelist):
        self.__lock.acquire(True)
        self.__data.append(self.__queueentry(outputfile, filelist))
        self.__lock.release()

    def pop(self):
        outputfile = None
        filelist = None
        self.__lock.acquire(True)
        if len(self.__data):
            nextentry = self.__data.pop(0)
        self.__lock.release()
        if nextentry:
            outputfile = nextentry.Outputfile
            filelist = nextentry.Filelist
        return outputfile, filelist

class Merger(threading.Thread):
    
    def __init__(self, workerID, workqueue):
        threading.Thread.__init__(self)
        self.__workerID = workerID
        self.__workqueue = workqueue

    def run(self):
        while True:
            outputfile, filelist = self.__workqueue.pop()
            if not outputfile or not filelist:
                break
            outputdir = os.path.dirname(outputfile)
            if not os.path.exists(outputdir):
                os.makedirs(outputdir, 0755)
            logging.info("Worker %d: Merging output file %s", self.__workerID, outputfile)
            ExecMerge(outputfile, filelist)
        logging.info("Worker %d: Finished work", self.__workerID)


def DoMerge(inputpath, filename):
    mergedir = "%s/merged" %(inputpath)
    if not os.path.exists(mergedir):
        os.makedirs(mergedir, 0755)

    queue = Workqueue()
    for pthard in sorted(os.listdir(inputpath)):
        if not pthard.isdigit():
            continue
        logging.info("Merging all file from pt-hard bin %s" %(pthard))
        outputdir = os.path.join(mergedir, pthard)
        queue.push_back(os.path.join(outputdir, filename), GetFilelist(os.path.join(inputpath, pthard), filename))
    
    workers = []
    for wid in range(0, 10):
        myworker = Merger(wid, queue)
        myworker.start()
    
    for worker in workers:
        worker.join()
    logging.info("Done")

if __name__ == "__main__":
    logging.basicConfig(format="[%(levelname)s] %(message)s", level = logging.INFO)
    inputpath = sys.argv[1]
    rootfile = "AnalysisResults.root"
    if len(sys.argv) > 2:
        rootfile = sys.argv[2]
    DoMerge(inputpath, rootfile)
