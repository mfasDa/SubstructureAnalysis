#! /usr/bin/env python

import commands
import getopt
import logging
import md5
import os
import subprocess
import sys
import threading
import zipfile

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

    def __extractZipfile(self, filename):
        if not ".zip" in filename or not os.path.exists(filename):
            return
        cwd = os.getcwd()
        os.chdir(os.path.dirname(filename))
        #unzip
        myarchive = zipfile.ZipFile(os.path.basename(filename))
        myarchive.extractall()
        os.chdir(cwd)

    def __waitforwork(self):
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
            self.__waitforwork()
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
                else:
                    # Copy successfull - extract the zipfile (if it is a zipfile)
                    self.__extractZipfile(nextfile.target())
            if not self.__poolfiller.isactive():
                # if pool is empty exit, else keep thread alive for remaining files
                if not self.__datapool.getpoolsize():
                hasWork = False

class PoolFiller(threading.Thread):
    
    def __init__(self, sample, trainrun, outputlocation, targetfile, maxpoolsize):
        threading.Thread.__init__(self)
        self.__datapool = None
        self.__alientool = None
        self.__isactive = False
        self.__sample = sample
        self.__trainrun = trainrun
        self.__outputlocation = outputlocation
        self.__targetfile = targetfile
        self.__maxpoolsize = maxpoolsize

    def setalientool(self, alientool):
        self.__alientool = alientool

    def setdatapool(self, datapool):
        self.__datapool = datapool

    def isactive(self):
        return self.__isactive
    
    def run():
        self.__isactive = True
        self.__find_files()
        self.__isactive = False

    def __wait(self):
        # wait until pool is half empty
        if self.__datapool.getpoolsize() < self.__maxpoolsize:
            return
        # pool full, wait until half empty
        emptylimit = self.__maxpoolsize/2
        while self.__datapool.getpoolsize() > emptylimit:
            time.sleep(5)

    def __find_files(self):
        run = 0
        ptharbin = 0
        for levelone in self.__alientool.listdir(self.__sample):
            # both run and pt-hard bin must be numeric
            if not levelone.isdigit():
                continue
            isrun = False
            if int(levelone) >= 190000:
                isrun = True
                run = int(levelone)
            else:
                isrun = False
                pthardbin = int(levelone)
            tmppath = os.path.join(self.__sample, levelone)
            logging.info("Doing %s %s", "run" if isrun else "pthard-bin", levelone)
            for leveltwo in self.__alientool.listdir(tmppath):
                if not leveltwo.isdigit():
                    continue
                if isrun:
                    pthardbin = int(leveltwo)
                else:
                    run = int(leveltwo)
                logging.info("Doing %s %s", "pthard-bin" if isrun else "run", leveltwo)
                tmppathtwo = os.path.join(tmppath, leveltwo)
                inputdir= os.path.join(tmppathtwo, self.__trainrun)
                inputfile = os.path.join(inputdir, self.__targetfile)
                outputdir = os.path.join(os.path.abspath(self.__outputlocation), "%02d" %pthardbin, "%d" %run)
                outputfile = os.path.join(outputdir, self.__targetfile)
                if not os.path.exists(outputfile):
                    self.__datapool.insert_pool(Filepair(inputfile, outputfile))
                    self.__wait()

def transfer(sample, trainrun, outputlocation, targetfile):
    datapool = DataPool()
    alientool = AlienTool()    

    poolfiller = PoolFiller(sample, trainrun, outputlocation, targetfile, 1000)
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
    print "Usage: ./copyFromGrid.py SAMPLE TRAINRUN OUTPUTPATH [OPTIONS]"
    print ""
    print "Arguments:"
    print "  SAMPLE:     Path in alien to the sample base directory"
    print "  TRAINRUN:   Full name of the train run (i. e. PWGJE/Jets_EMC_pp_MC/xxxx)"
    print "  OUTPUTPATH: Local directory where to write the output to"
    print ""
    print "Options:"
    print "  -f/--file=: Name of the file to be transferred (default: root_archive.zip)"
    print "  -d/--debug: Run with increased debug level"

if __name__ == "__main__":
    if len(sys.argv) < 4:
        usage()
        sys.exit(1)
    sample = sys.argv[1]
    trainrun = sys.argv[2]
    outputpath = str(sys.argv[3])
    outputfile = "root_archive.zip"
    debugging = False
    try:
        opt, arg = getopt.getopt(sys.argv[4:], "f:d", ["file=", "debug"])
        for o, a in opt:
            if o in ("-d", "--debug"):
                debugging = True
            if o in ("-f", "--file"):
                outputfile = a
    except getopt.getopterror as e:
        sys.exit(1)
    loglevel=logging.INFO
    if debugging:
        loglevel = logging.DEBUG
    logging.basicConfig(format='[%(levelname)s]: %(message)s', level=loglevel)
    transfer(sample, trainrun, outputpath, outputfile)
