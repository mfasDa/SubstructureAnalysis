#! /usr/bin/env python

import argparse
import logging
import os
import subprocess

class Testcase:

    def __init__(self, name, subdir):
        self.__name = name
        self.__datasubdir = subdir

    def getname(self):
        return self.__name
    
    def getdatasubdir(self):
        return self.__datasubdir

class Testrunner:

    def __init__(self, repo, basedir, macro):
        self.__repo = repo
        self.__basedir = basedir
        self.__macro = macro
        self.__tests = []
        self.__triggers = []
        self.__jetradii = []

    def addtest(self, testcase):
        self.__tests.append(testcase)

    def settriggers(self, triggers):
        self.__triggers = triggers
    
    def setjetradii(self, jetradii):
        self.__jetradii = jetradii

    def runall(self):
        # check whether macro is there  
        logging.info("Checking whether macro exist")
        if not os.path.exists(t.getmacro()):
            logging.info("Unfolding macro not found: %s", self.__macro)
            return
        logging.info("Macro check successfull: %s found", self.__macro)
        for t in self.__testcases:
            self.__runtest(t)
    
    def __runtest(self, testcase):
        logging.info("Running test: %s", testcase.getname())
        outputdir = os.path.join(self.__basedir, testcase.getdatasubdir())
        os.chdir(testcase.getoutputpath())
        mergedir_mc = "merged"
        for trg in self.__triggers:
            mergedir_data = "merged_1617" if trg == "INT7" else "merged_17"
            logging.info("Unfolding trigger: %s", trg)
            for r in self.__jetradii:
                logging.info("Unfolding Radius: %d", r)
                filename_data = "JetSubstructureTree_FullJets_R%02d_%s.root" %(r, trg)
                filename_mc = "JetSubstructureTree_FullJets_R%02d_%s_merged.root" %(r, trg)
                logfile_unfolding = "logunfolding_R%02d_%s.log" %(r, trg)
                datafile = os.path.join(os.getcwd(), "data", mergedir_data, filename_data)
                mcfile = os.path.join(os.getcwd(), "mc", mergedir_mc, filename_mc)
                if not os.path.exists(datafile):
                    logging.error("Data file %s not found", datafile)
                    continue
                if not os.path.exists(mcfile):
                logging.error("MC file %s not found", mcfile)
                    continue
                command = "root -l -b -q \'%s(\"%s\", \"%s\")\' | tee %s" % (os.path.join(self.__repo, "unfolding", self.__macro), datafile, mcfile, logfile_unfolding)
                subprocess.call(command, shell = True)
        # Run all plotters for the test case
        logfile_monitor = "logmonitor_R%02d.log" %(r)
        subprocess.call("%s/compareall.py zg | tee %s" %(self.__repo, logfile_monitor), shell = True)
        subprocess.call("%s/runiterall.py zg | tee %s" %(self.__repo, logfile_monitor), shell = True)
        subprocess.call("%s/sortPlotsComp.py | tee %s" %(self.__repo, logfile_monitor), shell = True)
        subprocess.call("%s/sortPlotsIter.py | tee %s" %(self.__repo, logfile_monitor), shell = True)

if __name__ == "__main__":
    defaulttests = ["trackingefff", "emcalseed", "emcaltimeloose", "emcaltimestrong"]
    logging.basicConfig(format='[%(levelname)s]: %(message)s', level=logging.INFO)
    repo = os.path.dirname(os.path.abspath(sys.argv[0]))
    parser = argparse.ArgumentParser(prog="detectorSystematics.py", description="Running detector systematics for zg")
    parser.add_argument("database", metavar="DATADIR", type=str, help="Directory where to find the data")
    parser.add_argument("-t", "--testcases", nargs='*', required=False, help="tests to be run (if not set all tests will be run")
    parser.add_argument("-d", "--dryrun", action="store_true", help="Set dry run mode")
    args = parser.parse_args()
    dryrun = args.dryrun
    database = os.path.abspath(args.database)
    currentdir = os.getcwd()
    testsrequired = [] 
    if args.testcases and len(args.testcases):
        testsrequired = args.testcases
    else:
        testsrequired = defaulttests
    testmanager = TestRunner(repo, database, "RunUnfoldingZgV1.cpp")
    testmanager.definetriggers(["INT7", "EJ1", "EJ2"])
    testmanager.defineradii([x for x in range(2, 6)])
    testcases = {"truncation" : Testcase("truncation", os.path.join(repo, "RunUnfoldingZgSys_truncation.cpp"), os.path.join(outputbase, "truncation"), ["loose", "strong"]),
                 "binning" : Testcase("binning", os.path.join(repo, "RunUnfoldingZgSys_binning.cpp"), os.path.join(outputbase, "binning"), ["option1", "option2", "option3", "option4"]),
                 "priors" : Testcase("priors", os.path.join(repo, "RunUnfoldingZgSys_priors.cpp"), os.path.join(outputbase, "priors"), ["default"]),
                 "closure" : Testcase("closure", os.path.join(repo, "RunUnfoldingZg_weightedClosure.cpp"), os.path.join(outputbase, "closure"), ["standard", "smeared"])}
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