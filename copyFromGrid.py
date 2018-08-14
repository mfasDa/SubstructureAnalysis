#! /usr/bin/env python

from __future__ import print_function

import commands
import getopt
import getpass
import hashlib
import logging
import md5
import os
import subprocess
import sys
import time
import threading
import zipfile

class AlienToken(object):
    
    def __init__(self, hostname = None, port = None, port2 = None, user = None, pwd = None, nonce = None, sid = None, enc = None, expdate = None):
        self.__hostname = hostname
        self.__port = port
        self.__port2 = port2
        self.__user = user
        self.__pwd = pwd
        self.__nonce = nonce
        self.__sid = sid
        self.__enc = enc
        self.__expdate = self.__convertstructtm(expdate) if expdate else None
    
    def __str__(self):
        hostnamestr = self.__hostname if self.__hostname else "None"
        portstr = "%d" %(sef.__port) if self.__port else "None"
        port2str = "%d" %(sef.__port2) if self.__port else "None"
        userstr = self.__user if self.__user else "None"
        pwdstr = self.__pwd if self.__pwd else "None"
        noncestr = self.__nonce if self.__nonce else "None"
        sidstr = "%d" %(self.__sid) if self.__sid else "None"
        encstr = "%d" %(self.__enc) if self.__enc else "None"
        expstr = time.strftime("%a, %d %b %Y, %H:%M:%S", self.__expdate) if self.__expdate else "None"
        return "Host: %s, Port: %s, Port2: %s, User: %s, Pwd: %s, Nonce: %s, SID: %s, Enc %s, Exp. date: %s" %(hostnamestr, portstr, port2str, userstr, pwdstr, noncestr, sidstr, encstr, expstr)

    def sethost(self, hostname):
        self.__hostname = hostname

    def gethost(self):
        return self.__hostname

    def setport(self, port):
        self.__port = port

    def getport(self):
        return self.__port

    def setport2(self, port2):
        self.__port2 = port2

    def getport2(self):
        return self.__port2
    
    def setuser(self, user):
        self.__user = user

    def getuser(self):
        return self.__user

    def setpwd(self, pwd):
        self.__pwd = pwd

    def getpwd(self):
        return self.__pwd

    def setnonce(self, nonce):
        self.__nonce = nonce
    
    def getnonce(self):
        return self.__nonce

    def setsid(self, sid):
        self.__sid = sid
    
    def getsid(self):
        return self.__sid

    def setenc(self, enc):
        self.__enc = enc

    def getenc(self):
        return self.__enc
    
    def setexpdate(self, expdate):
        self.__expdate = self.__convertstructtm(expdate)

    def getexpdate(self):
        return self.__expdate

    def __convertstructtm(self, timestring):
        try:
            return time.strptime(timestring, "%a %b %d %H:%M:%S %Y")
        except ValueError as e:
            logging.error("Error parsing exp time string: %s" %e)
            return None

    def isvalid(self):
        if not self.__expdate:
            return False
        now = time.localtime()
        tdiff = time.mktime(self.__expdate) - time.mktime(now)
        if tdiff > 0:
            return True
        return False
    
    Hostname = property(gethost, sethost)
    Port = property(getport, setport)
    Port2 = property(getport2, setport2)
    User = property(getuser, setuser)    
    Pwd = property(getpwd, setpwd)
    Nonce = property(getnonce, setnonce)
    Sid = property(getsid, setsid)
    Enc = property(getenc, setenc)
    Expdate = property(getexpdate, setexpdate)

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

    def fetchtokeninfo(self):
        outstrings = subprocess.check_output("alien-token-info").split("\n")
        token = AlienToken()
        for info in outstrings:
            if not ":" in info:
                continue
            key = info[:info.find(":")].rstrip().lstrip()
            value = info[info.find(":")+1:].rstrip().lstrip() 
            if not len(value):
                continue
            if key == "Host":
                token.Hostname = value
            elif key == "Port":
                token.Port = int(value)
            elif key == "Port2":
                token.Port2 = int(value)
            elif key == "User":
                token.User = value
            elif key == "Pwd":
                token.Pwd = value
            elif key == "Nonce":
                token.Nonce = value
            elif key == "SID":
                token.Sid = int(value)
            elif key == "Enc.Rep":
                token.Enc = int(value)
            elif key == "Expires":
                token.Expdate = value
        return token


    def handletoken(self):
        result = self.checktoken()
        ntrials = 0
        while not result:
            if ntrials >= 5:
                break
            logging.info("No valid token found, trying to create a new one ... (trial %s/5)" %(ntrials+1))
            self.renewtoken()
            result = self.checktoken()
            ntrials += 1
        return result
    
    def checktoken(self):
        token = self.fetchtokeninfo()
        logging.debug("Checking token:: %s" %(str(token)))
        if not token.isvalid():
            # Check for expired token
            return False
        return True

    def renewtoken(self):
        subprocess.call("alien-token-init %s" %(getpass.getuser()), shell=True)

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
    
    def run(self):
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

    def __get_trainid(self):
        idstring = self.__trainrun[self.__trainrun.rfind("/")+1:]
        return int(idstring.split("_")[0])

    def __get_legotrain(self):
        return self.__trainrun[0:self.__trainrun.rfind("/")] 

    def __extract_trainid(self, directory):
        return int(directory.split("_")[0])

    def __exist_traindir(self, legotrain, rundir):
        pwg = legotrain[0:legotrain.find("/")]
        traintype = legotrain[legotrain.find("/")+1:]
        pwgs = self.__alientool.listdir(rundir)
        if not pwg in pwgs:
            return False
        if not traintype in self.__alientool.listdir(os.path.join(rundir, pwg)):
            return False
        return True

    def __find_files(self):
        run = 0
        ptharbin = 0
        trainid = self.__get_trainid()
        legotrain = self.__get_legotrain()
        logging.info("Train ID: %d, lego train %s", trainid, legotrain)
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
                if not self.__exist_traindir(legotrain, tmppathtwo):
                    logging.info("No train %s found in %s", legotrain, tmppathtwo)
                    continue
                legotrainsdir = os.path.join(tmppathtwo, legotrain)
                mylegotrain = ""
                for t in self.__alientool.listdir(legotrainsdir):
                    logging.debug("traindir %s", t) 
                    if self.__extract_trainid(t) == trainid:
                        mylegotrain = t
                        break
                logging.debug("Extracted train run %s", mylegotrain)
                inputfile = os.path.join(legotrainsdir, mylegotrain, self.__targetfile)
                outputdir = os.path.join(os.path.abspath(self.__outputlocation), "%02d" %pthardbin, "%d" %run)
                outputfile = os.path.join(outputdir, self.__targetfile)
                if not os.path.exists(outputfile):
                    self.__datapool.insert_pool(Filepair(inputfile, outputfile))
                    self.__wait()
                else:
                    # output file existing, check if nor corrupted
                    if not self.__alientool.checkconsistency(inputfile, outputfile):
                        # local file corrupted - retry transferring
                        logging.info("Re-transferring file %s because of mismatch in MD5 sum", inputfile)
                        self.__datapool.insert_pool(Filepair(inputfile, outputfile))
                        self.__wait()

def transfer(sample, trainrun, outputlocation, targetfile, nstream):
    alientool = AlienTool()    
    if not alientool.handletoken():
        logging.error("No valid token found. Please execute \"alien-token-init\" first")
        sys.exit(2)
    datapool = DataPool()

    poolfiller = PoolFiller(sample, trainrun, outputlocation, targetfile, 1000)
    poolfiller.setdatapool(datapool)
    poolfiller.setalientool(alientool)
    poolfiller.start()

    copyworkers = []
    for i in range(0, nstream):
        worker = CopyHandler()
        worker.setdatapool(datapool)
        worker.setpoolfiller(poolfiller)
        worker.setalienhelper(alientool)
        worker.setmaxtrials(5)
        worker.start()
        copyworkers.append(worker)

    # join all threads
    poolfiller.join()
    for worker in copyworkers:
        worker.join()


def usage():
    print("Usage: ./copyFromGrid.py SAMPLE TRAINRUN OUTPUTPATH [OPTIONS]")
    print("")
    print("Arguments:")
    print("  SAMPLE:     Path in alien to the sample base directory")
    print("  TRAINRUN:   Full name of the train run (i. e. PWGJE/Jets_EMC_pp_MC/xxxx)")
    print("  OUTPUTPATH: Local directory where to write the output to")
    print("")
    print("Options:")
    print("  -f/--file=:    Name of the file to be transferred (default: root_archive.zip)")
    print("  -d/--debug:    Run with increased debug level")
    print("  -n/--nstream=: Number of parallel streams (default: 4)")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        usage()
        sys.exit(1)
    sample = sys.argv[1]
    trainrun = sys.argv[2]
    outputpath = str(sys.argv[3])
    outputfile = "root_archive.zip"
    debugging = False
    nstream = 4
    try:
        opt, arg = getopt.getopt(sys.argv[4:], "f:dn:", ["file=", "debug", "nstream="])
        for o, a in opt:
            if o in ("-d", "--debug"):
                debugging = True
            if o in ("-f", "--file"):
                outputfile = a
            if o in ("-n", "--nstream"):
                nstream = int(a)
    except getopt.getopterror as e:
        sys.exit(1)
    loglevel=logging.INFO
    if debugging:
        loglevel = logging.DEBUG
    logging.basicConfig(format='[%(levelname)s]: %(message)s', level=loglevel)
    transfer(sample, trainrun, outputpath, outputfile, nstream)