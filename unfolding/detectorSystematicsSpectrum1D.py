#! /usr/bin/env python

import argparse
import logging
import os
import subprocess
import sys
import threading

class Workqueue:

    def __init__(self):
        self.__tasks = []
        self.__lock = threading.Lock()
    
    def addtask(self, task):
        self.__lock.acquire(True)
        self.__tasks.append(task)
        self.__lock.release()

    def pop(self):
        task = None
        self.__lock.acquire(True)
        if len(self.__tasks):
            task = self.__tasks.pop(0)
        self.__lock.release()
        return task

class Taskrunner(threading.Thread):

    def __init__(self, workqueue):
        threading.Thread.__init__(self)
        self.__workqueue = workqueue

    def run(self):
        task = self.__workqueue.pop()
        while task:
            subprocess.call(task, shell=True)
            task = self.__workqueue.pop() 


class Testcase:

    def __init__(self, name, subdir):
        self.__name = name
        self.__datasubdir = subdir

    def getname(self):
        return self.__name
    
    def getdatasubdir(self):
        return self.__datasubdir

class TestRunner:

    def __init__(self, repo, basedir, datadir, macro):
        self.__repo = repo
        self.__basedir = basedir
        self.__datadir = datadir
        self.__macro = macro
        self.__nworkers = 1
        self.__tests = []
        self.__triggers = []
        self.__jetradii = []

    def addtest(self, testcase):
        self.__tests.append(testcase)

    def settriggers(self, triggers):
        self.__triggers = triggers
    
    def setjetradii(self, jetradii):
        self.__jetradii = jetradii
    
    def setnworkers(self, nworkers):
        self.__nworkers = nworkers

    def runall(self):
        # check whether macro is there  
        logging.info("Checking whether macro exist")
        if not os.path.exists(os.path.join(self.__repo, self.__macro)):
            logging.info("Unfolding macro not found: %s, repo %s", self.__macro, self.__repo)
            return
        logging.info("Macro check successfull: %s found", self.__macro)
        for t in self.__tests:
            self.__runtest(t)
    
    def __runtest(self, testcase):
        logging.info("Running test: %s", testcase.getname())
        outputdir = os.path.join(self.__basedir, testcase.getname())
        if not os.path.exists(outputdir):
            os.makedirs(outputdir, 0755)
        os.chdir(outputdir)
        workqueue = Workqueue()
        for r in self.__jetradii:
            logging.info("Unfolding Radius: %f", float(r)/10.)
            logfile_unfolding = "logunfolding_R%02d.log" %(r)
            command = "root -l -b -q \'%s(%f, \"%s\")\' | tee %s" % (os.path.join(self.__repo, self.__macro), float(r)/10., os.path.join(self.__datadir, testcase.getdatasubdir()), logfile_unfolding)
            workqueue.addtask(command)
        
        tasks = []
        for itask in range(0, self.__nworkers):
            systask = Taskrunner(workqueue)
            systask.start()
            tasks.append(systask)
        for task in tasks:
            task.join()

if __name__ == "__main__":
    defaulttests = ["trackingeff", "emcalseed", "emcaltimeloose", "emcaltimestrong", "emcalclusterizerv1", "emcalhadcorrf0"]
    logging.basicConfig(format='[%(levelname)s]: %(message)s', level=logging.INFO)
    repo = os.path.dirname(os.path.abspath(sys.argv[0]))
    parser = argparse.ArgumentParser(prog="detectorSystematics.py", description="Running detector systematics for zg")
    parser.add_argument("database", metavar="DATADIR", type=str, help="Directory where to find the data")
    parser.add_argument("-t", "--testcases", nargs='*', required=False, help="tests to be run (if not set all tests will be run")
    parser.add_argument("-d", "--dryrun", action="store_true", help="Set dry run mode")
    args = parser.parse_args()
    dryrun = args.dryrun
    database = os.path.abspath(args.database)
    basedir = os.getcwd()
    currentdir = os.getcwd()
    testsrequired = [] 
    if args.testcases and len(args.testcases):
        testsrequired = args.testcases
    else:
        testsrequired = defaulttests
    testmanager = TestRunner(repo, basedir, database, "runCorrectionChain1DBayes.cpp")
    testmanager.settriggers(["INT7", "EJ1", "EJ2"])
    testmanager.setjetradii([x for x in range(2, 6)])
    testcases = {"trackingeff" : Testcase("trackingeff", "20180823_trackingeff"),
                 "emcalseed" : Testcase("emcalseed", "20180813_emchighthresh"),
                 "emcaltimeloose" : Testcase("emcaltimeloose", "20180823_emcalloosetimecut"),
                 "emcaltimestrong" : Testcase("emcaltimestrong", "20180823_emcalstrongtimecut"),
                 "emcalclusterizerv1" : Testcase("emcalclusterizerv1", "20180907_clusterizerv1"),
                 "emcalhadcorrf0" : Testcase("emcalhadcorrf0", "20180907_hadcorrf0")}
    caselogger = lambda tc : logging.info("Adding test case \"%s\"", tc)
    testadder = lambda tc : testmanager.addtest(testcases[tc]) if not dryrun else logging.info("Not adding test due to dry run")
    for t in defaulttests:
        if t in testsrequired:
            caselogger(t)
            testadder(t)
    if not dryrun:
        testmanager.runall()
        os.chdir(currentdir)
    logging.info("Done")