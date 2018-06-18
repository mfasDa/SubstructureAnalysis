#! /usr/bin/env python

import commands
import getopt
import logging
import md5
import os
import subprocess
import sys
import threading

class AlienTool: 
    
    def __init__(self):
        self.__lock = threading.Lock()
  
    def md5(self, fname):
        hash_md5 = hashlib.md5()
        with open(fname, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
               hash_md5.update(chunk)
        return hash_md5.hexdigest()

    def gridmd5sum(self, gridfile):
        # Function must be robust enough to fetch all possible
        # xrd error states which it usually gets from the stdout
        # of the query process
        gbout = ''
        errorstate = True
        while errorstate:
            errorstate = False
            gbout = commands.getstatusoutput("gbbox md5sum %s" %gridfile)[1]
            if gbout.startswith("Error") or gbout.startswith("Warning") or "CheckErrorStatus" in gbout:
                errorstate = True
        return gbout.split('\t')[0]

    def checkconsistency(self, gridfile, localfile):
        gmd5 = self.gridmd5sum(gridfile)
        lmd5 = self.md5(localfile)
        if lmd5 == gmd5:
            return True
        return False

    def copy(self, inputfile, outputfile):
        logging.info("Copying %s to %s", inputfile, outputfile)
        self.__lock.acquire(True)
        if not os.path.exists(os.path.dirname(outputfile)):
            os.makedirs(os.path.dirname(outputfile), 0755)
        self.__lock.release()
        subprocess.call(['alien_cp', 'alien://%s'%inputfile, outputfile])
        # make check
        if os.path.exists(outputfile):
            localmd5 = self.md5(outputfile)
            gridmd5 = self.gridmd5sum(inputfile)
            if localmd5 != gridmd5:
                logging.error("Mismatch in MD5 sum for file %s", outputfile)
                # incorrect MD5sum, outputfile probably corrupted
                os.remove(outputfile)
                return False
            else:
                logging.info("Output file %s copied correctly", outputfile)
                return True
        else:
            logging.error("output file %s not found" %outputfile)
            return False

    def listdir(self, inputdir):
        # Function must be robust agianst error states which
        # it can only get from the stdout. As long as the 
        # request ends in error state it should retry
        errorstate = True
        while errorstate:
            dirs = commands.getstatusoutput("alien_ls %s" %inputdir)[1]
            errorstate = False
            result = []
            for d in dirs.split("\n"):
                if d.startswith("Error") or d.startswith("Warning"):
                    errorstate = True
                    break
                mydir = d.rstrip().lstrip()
                if len(mydir):
                    result.append(mydir)
            if errorstate:
                continue
            return result

class Filepair:
  
    def __init__(self, source, target, ntrials = 0):
        self.__source = source
        self.__target = target
        self.__ntrials = ntrials

    def setntrials(self, ntrials):
        self.__ntrials = ntrials

    def getntrials(self):
        return self.__ntrials

    def source(self):
        return self.__source

    def target(self):
        return self.__target

class DataPool :
  
    def __init__(self):
        self.__data = []
        self.__lock = threading.Lock()

    def insert_pool(self, filepair):
        self.__lock.acquire(True)
        logging.info("Adding to pool: %s - %s", filepair.source(), filepair.target())
        self.__data.append(filepair)
        self.__lock.release()

    def getpoolsize(self):
        return len(self.__data)

    def pop(self):
        result = None
        self.__lock.acquire(True)
        if(self.getpoolsize()):
            result = self.__data.pop(0)
        self.__lock.release()
        return result

class CopyHandler(threading.Thread):
  
    def __init__(self):
        threading.Thread.__init__(self)
        self.__datapool = None
        self.__poolfiller = None
        self.__alienhelper = None
        self.__maxtrials = 5
  
    def setalienhelper(self, alienhelper):
        self.__alienhelper = alienhelper
  
    def setdatapool(self, datapool):
        self.__datapool = datapool

    def setpoolfiller(self, poolfiller):
        self.__poolfiller = poolfiller

    def setmaxtrials(self, maxtrials):
        self.__maxtrials = maxtrials

    def waitforwork(self):
        if self.__datapool.getpoolsize():
            return
        if not self.__poolfiller.isactive():
            return
        while not self.__datapool.getpoolsize():
            if not self.__poolfiller.isactive():
            break
        time.sleep(5)

    def run(self):
        hasWork = True
        while hasWork:
            self.waitforwork()
            nextfile = self.__datapool.pop()
            if nextfile:
                copystatus = self.__alienhelper.copy(nextfile.source(), nextfile.target())
                if not copystatus:
                    # put file back on the pool in case of copy failure
                    # only allow for amaximum amount of copy trials
                    trials = nextfile.getntrials()
                    trials += 1
                    if trials >= self.__maxtrials:
                        logging.error("File %s failed copying in %d trials - giving up", nextfile.source(), self.__maxtrials)
                    else:
                        logging.error("File %s failed copying (%d/%d) - re-inserting into the pool ...", nextfile.source(), trials, self.__maxtrials)
                        nextfile.setntrials(trials)
                        self.__datapool.insert_pool(nextfile)
            if not self.__poolfiller.isactive():
                # if pool is empty exit, else keep thread alive for remaining files
                if not self.__datapool.getpoolsize():
                hasWork = False

class PoolFiller(threading.thread):
    
    def __init__(self, outputpath, trainname, trainid, filestocopy):
        threading.Thread.__init__(self)
        self.__datapool = None
        self.__alientool = None
        self.__isacvtive = False
        self.__outputpath = outputpath
        self.__trainname = trainname
        self.__trainid = trainid
        self.__filestocopy = filestocopy

    def __wait(self):
        # wait until pool is half empty
        if self.__datapool.getpoolsize() < self.__maxpoolsize:
            return
        # pool full, wait until half empty
        emptylimit = self.__maxpoolsize/2
        while self.__datapool.getpoolsize() > emptylimit:
            time.sleep(5)

    def __getchildpaths(self, inputdir, trainid):
        childdirs = filter(lambda n : n.split("_")[0] == trainid and "child" in n, self.__alientool.listdir(inputdir))
        return map(lambda c : os.path.join(inputdir, c), childdirs)

    def getchildnumber(inputpath):
        if not "child" in inputpath:
            return 0
        return int(os.path.basename(inputpath).split("_")[3])

    def run(self):
        self.__isacvtive = True
        outfilenames = self.__filestocopy.split(",")
        traindir = os.path.join("/alice/cern.ch/user/a/alitrain", self.__trainname)
    
        if not os.path.exists(self.__outputpath):
            os.makedirs(self.__outputpath)

        for childpath in self.__getchildpaths(self.__traindir, trainid):
            child_id = self.__getchildnumber(childpath)
            logging.info("Processing child %d", child_id)

            locchilddir = os.path.join(self.__outputpath, "child_%d" %child_id)
            if not os.path.exists(locchilddir):
                os.makedirs(locchilddir, 0755)

            for transferfile in outfilenames:
                alienfile = os.path.join(childpath, "merge", transferfile)
                localfile = os.path.join(locchilddir, transferfile)
                if os.path.exists(localfile) and self.__alientool.checkconsistency(alienfile, localfile):
                    logging.info("Not copying %s because it is already found locally", alienfile)
                    continue
                logging.info("Copying %s to %s", alienfile, localfile);
                self.__datapool.insert_pool(Filepair(alienfile, localfile))
                self.__wait()
        self.__isacvtive = False

def main(ooutputpath, trainname, trainid, filestocpy):
    alientool = Alientool()
    pool = DataPool()

    poolfiller = PoolFiller(ooutputpath, trainname, trainid, filestocpy)
    poolfiller.setdatapool(datapool)
    poolfiller.setalientool(alientool)
    poolfiller.start()
    
    copyworkers = []
    for i in range(0, 4):
        worker = CopyHandler()
        worker.setdatapool(datapool)
        worker.setpoolfiller(poolfiller)
        worker.setalienhelper(alientool)
        worker.setmaxtrials(5)
        worker.start()
        copyworkers.append(worker)

    # join all threads
    poolfiller.join()
    for worker in workers:
        worker.join()

def usage():
    print "Usage:"
    print ""
    print "./copyFromGridMeta.py OUTPUTPATH TRAINNAME TRAINID FILESTOCOPY [OPTIONS]"
    print ""
    print "Arguments:"
    print "  OUTPUTPATH:  Path where to store the files locally"
    print "  TRAINNAME:   Name of the lego train (i.e. PWGJE/Jets_EMC_pp)"
    print "  TRAINID:     ID of the train run (only ID number needed, no full tag)"
    print "  FILESTOCOPY: Comma-separated list of files to be copied"
    print ""
    print "Options:"
    print "  -d/--debug:   Add debug logging"
    print ""
    print "After copy the files will be arraged as"
    print ""
    print "  OUTPUTPATH/child_x/file1"
    print "  OUTPUTPATH/child_x/file2"
    print "  ..."

if __name__ == "__main__":
    if len(sys.argv) < 5:
        usage()
        sys.exit(1)
    OUTPUTDIR   = sys.argv[1]
    TRAINNAME   = sys.argv[2]
    TRAINID     = sys.argv[3]
    FILESTOCOPY = sys.argv[4]
    debugging = False
    try:
        opt, arg = getopt.getopt(sys.argv[4:], "d", ["debug"])
        for o, a in opt:
            if o in ("-d", "--debug"):
                debugging = True
    except getopt.getopterror as e:
        sys.exit(1)
    loglevel=logging.INFO
    if debugging:
        loglevel = logging.DEBUG
    logging.basicConfig(format='[%(levelname)s]: %(message)s', level=loglevel)
    main(OUTPUTDIR, TRAINNAME, TRAINID, FILESTOCOPY)