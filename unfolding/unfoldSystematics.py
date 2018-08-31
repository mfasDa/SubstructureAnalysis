#! /usr/bin/env python

from __future__ import print_function
import logging
import os
import subprocess
import sys

class Testcase:

    def __init__(self, name, marcro, outputpath):
        self.__name = name
        self.__macro = macro
        self.__outputpath = outputpath

    def getname(self):
        return self.__name

    def getmacro(self):
        return self.__macro

    def getoutputpath(self):
        return self.__outputpath

class TestRunner:

    def __init__(self, datarepo, mcrepo, coderepo):
        self.__datarepo = datarepo
        self.__mcrepo = mcrepo
        self.__coderepo = coderepo
        self.__testcases = []
        self.__triggers = []
        self.__jetradii = []
    
    def definetriggers(self, triggers):
        self.__triggers = triggers

    def defineradii(self, jetradii):
        self.__jetradii = jetradii

    def addtest(self, testcase):
        self.__testcases = testcase

    def runall(self):
        if not os.path.exists()        

    def __runtest(self, testcase):
        logging.info("Running test: %s" testcase.getname())
        if not os.path.exists():
            os.makedirs(testcase.getoutputpath())
        os.chdir(testcase.getoutputpath())
        mergedir_mc = "merge_barrel"
        for trg in self._triggers:
            mergedir_data = "merge_1617" if trigger == "INT7" else "merge_17"
            logging.info("Unfolding trigger: %s", trg)
            for r in self.__jetradii:
                logging.info("Unfolding Radius: %d", r)
                filename = "JetSubstructureTree_FullJets_R%02d_%s.root" %(r, trg)
                datafile = os.path.join(self.__datarepo, mergedir_data, filename)
                mcfile = os.path.join(self.__mcrepo, mergedir_mc, filename)
                command = "root -l -b -q \'%s(\"%s\", \"%s\")\'" % (testcase.getmacro(), datafile, mcfile)
                subprocess.call(command, shell = True)
        # Run all plotters for the test case
        subprocess.call("%s/compareall.py zg" %repo, shell = True)
        subprocess.call("%s/runiterall.py zg" %repo, shell = True)
        subprocess.call("%s/sortPlotsComp.py" %repo, shell = True)
        subprocess.call("%s/sortPlotsIter.py" %repo, shell = True)


if __name__ == "__main__":
    repo = os.path.dirname(os.path.abspath(sys.argv[0]))
    database = os.path.abspath(sys.argv[1])
    outputbase = os.path.abspath(sys.argv[2])
    currentdir = os.getcwd()
    testmanager = TestRunner(os.path.join(database, "data"), os.path.join(database, "mc"), repo)
    testmanager.definetriggers(["INT7", "EJ1", "EJ2"])
    testmanager.defineradii([x for x in range(2, 6)])
    testmanager.addtest(Testcase("truncation", os.path.join(repo, "RunUnfoldingZgSys_Truncation.cpp"), os.path.join(outputbase, "truncation")))
    testmanager.addtest(Testcase("binning", os.path.join(repo, "RunUnfoldingZgSys_Binninng.cpp"), os.path.join(outputbase, "binning")))
    testmanager.addtest(Testcase("priors", os.path.join(repo, "RunUnfoldingZgSys_Priors.cpp"), os.path.join(outputbase, "priors")))
    testmanager.addtest(Testcase("closure", os.path.join(repo, "RunUnfoldingZg_weightedClosure.cpp"), os.path.join(outputbase, "closure")))
    testmanager.runall()
    os.chdir(currentdir)