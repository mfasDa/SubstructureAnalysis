#! /usr/bin/env python

from __future__ import print_function
import argparse
import logging
import os
import subprocess
import sys
import threading

class Workqueue:

    def __init__(self):
        self.__tasks = []
        self.__lock = threading.Lock
    
    def addtask(self, task):
        self.__lock.acquire(True)
        self.__tasks.append(task)
        self.__lock.release

    def pop(self):
        task = None
        self.__lock.aquire(True)
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

    def __init__(self, name, macro, outputpath, sysoptions, triggeroption = None, mcresponsetrigger = False):
        self.__name = name
        self.__macro = macro
        self.__outputpath = outputpath
        self.__sysoptions = sysoptions
        self.__triggeroption = triggeroption    
        self.__mcresponsetrigger = mcresponsetrigger

    def getname(self):
        return self.__name

    def getmacro(self):
        return self.__macro

    def getoutputpath(self):
        return self.__outputpath

    def getsysoptions(self):
        return self.__sysoptions

    def gettriggeroptions(self):
        return self.__triggeroption

    def isusemctriggerresponse(self):
        return self.__mcresponsetrigger

class TestRunner:

    def __init__(self, datarepo, mcrepo, coderepo):
        self.__datarepo = datarepo
        self.__mcrepo = mcrepo
        self.__coderepo = coderepo
        self.__nworkers = 1
        self.__testcases = []
        self.__triggers = []
        self.__jetradii = []
    
    def definetriggers(self, triggers):
        self.__triggers = triggers

    def defineradii(self, jetradii):
        self.__jetradii = jetradii

    def addtest(self, testcase):
        self.__testcases.append(testcase)

    def setnworkers(self, nworkers):
        self.__nworkers = nworkers

    def runall(self):
        # check whether macros are there  
        logging.info("Checking whether macros exist")
        ncheck = 0
        nmiss = 0
        for t in self.__testcases:
            found = True
            ncheck += 1
            if not os.path.exists(t.getmacro()):
                found = False
                nmiss += 1
            logging.info("Checking %s ... %s", t.getmacro(), "found" if found else "not found")
        logging.info("Macro check: Checked %d, failures %d", ncheck, nmiss)
        if nmiss > 0:
            logging.error("Macro test failed, test suite aborted")
            return
        else:
            logging.info("Macro test successfull, test suite can run")
            for t in self.__testcases:
                self.__runtest(t)

    def __runtest(self, testcase):
        logging.info("Running test: %s", testcase.getname())
        if not os.path.exists(testcase.getoutputpath()):
            os.makedirs(testcase.getoutputpath()), 0755
        os.chdir(testcase.getoutputpath())
        mergedir_mc = "merged_calo"
        for o in testcase.getsysoptions():
            sysdir = os.path.join(testcase.getoutputpath(), o)
            if not os.path.exists(sysdir):
                os.makedirs(sysdir, 0755)
            os.chdir(sysdir)
            logging.info("Running systematics: %s", o)

            workqueue = Workqueue()
            for trg in self.__triggers:
                testtriggeroptions = testcase.gettriggeroptions()
                if testtriggeroptions and len(testtriggeroptions):
                    if not trg in testtriggeroptions:
                        continue
                mergedir_data = "merged_1617" if trg == "INT7" else "merged_17"
                logging.info("Unfolding trigger: %s", trg)
                for r in self.__jetradii:
                    logging.info("Unfolding Radius: %d", r)
                    filename_data = "JetSubstructureTree_FullJets_R%02d_%s.root" %(r, trg)
                    filename_mc = "JetSubstructureTree_FullJets_R%02d_%s_merged.root" %(r, trg if testcase.isusemctriggerresponse() else "INT7")
                    logfile_unfolding = "logunfolding_R%02d_%s.log" %(r, trg)
                    datafile = os.path.join(self.__datarepo, mergedir_data, filename_data)
                    mcfile = os.path.join(self.__mcrepo, mergedir_mc, filename_mc)
                    if not os.path.exists(datafile):
                        logging.error("Data file %s not found", datafile)
                        continue
                    if not os.path.exists(mcfile):
                        logging.error("MC file %s not found", mcfile)
                        continue
                    command = "root -l -b -q \'%s(\"%s\", \"%s\", \"%s\")\' | tee %s" % (testcase.getmacro(), datafile, mcfile, o, logfile_unfolding)
                    workqueue.addtask(command)

            tasks = []
            for itask in range(0, self.__nworkers):
                systask = Taskrunner(workqueue)
                systask.start()
                tasks.append(systask)
            for task in tasks:
                task.join()
            # Run all plotters for the test case
            logfile_monitor = "logmonitor_R%02d.log" %(r)
            subprocess.call("%s/compareall.py zg | tee %s" %(self.__coderepo, logfile_monitor), shell = True)
            subprocess.call("%s/runiterall.py zg | tee %s" %(self.__coderepo, logfile_monitor), shell = True)
            subprocess.call("%s/sortPlotsComp.py | tee %s" %(self.__coderepo, logfile_monitor), shell = True)
            subprocess.call("%s/sortPlotsIter.py | tee %s" %(self.__coderepo, logfile_monitor), shell = True)


if __name__ == "__main__":
    defaulttests = ["truncation", "binning", "priors", "closure"]
    logging.basicConfig(format='[%(levelname)s]: %(message)s', level=logging.INFO)
    repo = os.path.dirname(os.path.abspath(sys.argv[0]))
    parser = argparse.ArgumentParser(prog="unfoldSystematics.py", description="Running unfolding systematics for zg")
    parser.add_argument("database", metavar="DATADIR", type=str, help="Directory where to find the data")
    parser.add_argument("outputbase", metavar="OUTDIR", type=str, help="Directory where to store the output")
    parser.add_argument("-t", "--testcases", nargs='*', required=False, help="tests to be run (if not set all tests will be run")
    parser.add_argument("-d", "--dryrun", action="store_true", help="Set dry run mode")
    parser.add_argument("-n", "--ntasks", type=int, default=4 help="Number of parallel workers")
    args = parser.parse_args()
    dryrun = args.dryrun
    database = os.path.abspath(args.database)
    outputbase = os.path.abspath(args.outputbase)
    currentdir = os.getcwd()
    testsrequired = [] 
    if args.testcases and len(args.testcases):
        testsrequired = args.testcases
    else:
        testsrequired = defaulttests
    testmanager = TestRunner(os.path.join(database, "data"), os.path.join(database, "mc"), repo)
    testmanager.definetriggers(["INT7", "EJ1", "EJ2"])
    testmanager.defineradii([x for x in range(2, 6)])
    testmanager.setnworkers(args.ntasks)
    testcases = {"truncation" : Testcase("truncation", os.path.join(repo, "RunUnfoldingZgSys_truncation.cpp"), os.path.join(outputbase, "truncation"), ["loose", "strong"]),
                 "binning" : Testcase("binning", os.path.join(repo, "RunUnfoldingZgSys_binning.cpp"), os.path.join(outputbase, "binning"), ["option1", "option2", "option3", "option4"]),
                 "priors" : Testcase("priors", os.path.join(repo, "RunUnfoldingZgSys_priors.cpp"), os.path.join(outputbase, "priors"), ["default"]),
                 "closure" : Testcase("closure", os.path.join(repo, "RunUnfoldingZg_weightedClosure.cpp"), os.path.join(outputbase, "closure"), ["standard", "smeared"])}
                 "triggerresponse" : Testcase("triggerresponse", os.path.join(repo, "RunUnfoldingZg.cpp", os.path.join(outputbase, "triggerresponse"), ["default"], triggeroption = ["EJ1", "EJ2"], mcresponsetrigger = True))
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