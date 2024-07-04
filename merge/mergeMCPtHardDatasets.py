#! /usr/bin/env python3

import argparse
import logging
import multiprocessing
import os
import subprocess
import threading

class PtHardBin:

    def __init__(self, binID):
       self.__binid = binID
       self.__files = [] 

    def __eq__(self, other):
        otherbinid = -1
        if isinstance(other, int):
            otherbinid = other
        elif isinstance(other, PtHardBin):
            otherbinid = other.getbinnumber()
        return self.__binid == otherbinid

    def getbinnumber(self):
        return self.__binid

    def addfile(self, file):
        self.__files.append(file)

    def merge(self, outputdir, basename):
        outputfile = os.path.join(outputdir, "%02d" %self.__binid, basename)
        if not os.path.exists(os.path.dirname(outputfile)):
            os.makedirs(os.path.dirname(outputfile), 0o755)
        command = ["hadd", "-f", outputfile]
        for f in self.__files:
            command.append(f)
        try:
            result = subprocess.call(command)
            logging.debug("Merge process finished with return code %d", result)
        except OSError as e:
            logging.error("Failed spawning merge process: %s", e)

class Workqueue:

    def __init__(self):
        self.__data = []
        self.__lock = threading.Lock()

    def insertpthardbin(self, pthardbin):
        self.__lock.acquire(True)
        self.__data.append(pthardbin)
        self.__lock.release()

    def pop(self):
        result = None
        self.__lock.acquire(True)
        if len(self.__data):
            result = self.__data.pop(0)
        self.__lock.release()
        return result

    def size(self):
        return len(self.__data)

class Merger(threading.Thread):

    def __init__(self, workerID, outputdir, basename, workqueue):
        threading.Thread.__init__(self)
        self.__workerID = workerID
        self.__workqueue = workqueue
        self.__outputdir = outputdir
        self.__basename = basename

    def run(self):
        logging.info("Worker %d started", self.__workerID)
        while True:
            nextbin = self.__workqueue.pop()
            if not nextbin:
                break
            logging.info("Worker %d: Merging pt-hard bin %d", self.__workerID, nextbin.getbinnumber())
            nextbin.merge(self.__outputdir, self.__basename)
        logging.info("Worker %d: Finished work", self.__workerID)

def mergemcptharddatasets(inputdir, basename, mergedir, nworkrequest):
    pthardbins = []

    logging.info("Base file: %s", basename)
    logging.info("Merge dir: %s", mergedir)

    outputdir = os.path.join(inputdir, mergedir)
    if not os.path.exists(outputdir):
        os.makedirs(outputdir)
    for production in [prod for prod in os.listdir(inputdir) if "LHC" in prod]:
        if production == os.path.basename(outputdir):
            continue
        if not os.path.isdir(production):
            continue
        inmerged = os.path.join(inputdir, production, mergedir)
        if not os.path.exists(inmerged):
            continue

        for binID in [int(x) for x in os.listdir(inmerged) if x.isdigit()]:
            prodfile = os.path.join(inmerged, "%02d" %binID, basename)
            if os.path.exists(prodfile):
                binindex = -1
                try:
                    binindex = pthardbins.index(binID)
                except ValueError as e:
                    pass
                pthardbin = None
                if binindex >= 0:
                    pthardbin = pthardbins[binindex]
                else:
                    pthardbin = PtHardBin(binID)
                    pthardbins.append(pthardbin)
                pthardbin.addfile(prodfile)
    logging.info("Found %d pt-hard bins", len(pthardbins))

    workqueue = Workqueue()
    for b in pthardbins:
        workqueue.insertpthardbin(b)
    logging.info("Workqueue has size %d", workqueue.size())

    workers = []
    nworkersused = max(min(nworkrequest, multiprocessing.cpu_count()-2), 1)
    logging.info("Using %d parallel mergers", nworkersused)
    for wid in range(0, nworkersused):
        logging.info("Starting merger %d", wid)
        #continue
        merger = Merger(wid, outputdir, basename, workqueue)
        merger.start()
        workers.append(merger)
  
    #return
    for merger in workers:
        merger.join()
    logging.info("Done")

if __name__ == "__main__":
    logging.basicConfig(format="[%(levelname)s]: %(message)s", level=logging.INFO)
    parser = argparse.ArgumentParser(prog = "./mergeMCPtHardDatasets.py", description = "Merging pt-hard bins from multiple pt-hard datasets")
    parser.add_argument("basedir", metavar="BASEDIR", help = "Directory where to find the productions")
    parser.add_argument("-f" , "--file", type = str, default = "AnalysisResults.root", help = "ROOT file to be merged (default: AnalysisResults.root)")
    parser.add_argument("-m", "--mergedir", type = str, default = "merged", help = "Directory of the period-merged output (default: merged)")
    parser.add_argument("-n", "--nworkers", type = int, default = multiprocessing.cpu_count(), help = "Number of parallel workers")
    args = parser.parse_args()
    mergemcptharddatasets(args.basedir, args.file, args.mergedir, args.nworkers)
