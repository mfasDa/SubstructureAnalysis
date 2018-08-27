#! /usr/bin/env python
from __future__ import print_function
import getopt
import logging
import multiprocessing
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

def GetFilelist(inputpath, filename, runlist = None):
    logging.debug("walking %s" %inputpath)
    result = []
    for root, dirs, files in os.walk(inputpath):
        for f in files:
            if filename in f:  
                fullfilename = os.path.join(root, f)
                isSelectedRun = True
                if runlist:
                    isSelectedRun = False
                    for r in runlist:
                        if str(r) in fullfilename:
                            logging.info("Selected %s for run %d" %(fullfilename, r))
                            isSelectedRun = True
                    if not isSelectedRun:
                        logging.info("No matching run found for %s" %fullfilename)
                if isSelectedRun:
                    result.append(os.path.join(root, fullfilename))
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
        nextentry = None
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


def DoMerge(inputpath, filename, runlist = None, nworkers = 10):
    mergedir = "%s/merged" %(inputpath)
    if not os.path.exists(mergedir):
        os.makedirs(mergedir, 0755)

    # prepare runlist (in case selected)
    runlistlist = None
    if runlist:
        runlistlist = []
        with open("%s/runlists_EMCAL/%s" %(os.path.dirname(os.path.abspath(sys.argv[0])), runlist)) as reader:
            for line in reader:
                line = line.rstrip().lstrip()
                tokens = line.split(",")
                for t in tokens:
                    if not len(t):
                        continue
                    runlistlist.append(int(t))
        logging.info("Using runlist %s:" %runlist)
        logging.info("==========================")
        logstr = str()
        first = True
        for r in runlistlist:
            if not first:
                logstr += ", "
            else:
                first = False
            logstr += str(r)
        logging.info(logstr)

    # Put merge files in queue
    queue = Workqueue()
    for pthard in sorted(os.listdir(inputpath)):
        if not pthard.isdigit():
            continue
        logging.info("Merging all file from pt-hard bin %s" %(pthard))
        outputdir = os.path.join(mergedir, pthard)
        queue.push_back(os.path.join(outputdir, filename), GetFilelist(os.path.join(inputpath, pthard), filename, runlistlist))
    
    # start workers
    nworkersused = max(min(nworkers, multiprocessing.cpu_count()-2), 1);
    logging.info("Using %d parallel mergers" %nworkersused)
    workers = []
    for wid in range(0, nworkersused):
        myworker = Merger(wid, queue)
        myworker.start()
    
    # wait for done
    for worker in workers:
        worker.join()
    logging.info("Done")

def usage():
    print("usage: ./mergeRuns.py OUTPUTDIR [OPTIONS]")
    print("")
    print("Merging all runs for a certain pt-hard bin,")
    print("looping over all pt-hard bins.")
    print("")
    print("Options:")
    print(" -f/--file=:     Name of the file to be merged")
    print(" -r/--runlist=:  Use runlist for merging")
    print(" -n/--nworkers=: Number of parallel mergers (default: number of CPU - 2, max: Number of CPU - 2")
    print(" -h/--help:      Printing usage instructions")

if __name__ == "__main__":
    logging.basicConfig(format="[%(levelname)s] %(message)s", level = logging.INFO)
    if(len(sys.argv) < 2):
        logging.error("Too few arguments")
        usage()
        sys.exit(1)
    inputpath = sys.argv[1]
    rootfile = "AnalysisResults.root"
    runlist = None
    nworkers = None
    if len(sys.argv) > 2:
        try:
            opt,arg = getopt.getopt(sys.argv[2:], "f:n:r:h", ["file=", "nworkers=", "runlist=", "help"])
            for o,a in opt:
                if o in ("-f", "--file"):
                    rootfile = a
                elif o in ("-r", "--runlist"):
                    runlist = a
                elif o in ("-n", "--nworkers"):
                    nworkers =  int(a)
                elif o in ("-h", "--help"):
                    usage()
                    sys.exit(1)
        except getopt.GetoptError as e:
            logging.error("Invalid option: %s" %e)
            usage()
            sys.exit(1)
    if nworkers:
        DoMerge(inputpath, rootfile, runlist, nworkers)
    else:
        DoMerge(inputpath, rootfile, runlist)
