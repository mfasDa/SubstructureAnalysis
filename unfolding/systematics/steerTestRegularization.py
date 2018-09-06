#! /usr/bin/env python
import argparse
import logging
import os
import shutil
import sys
import subprocess

class Variation:

    def __init__(self, name, title, value, defaultvalue):
        self.__name = name
        self.__title = title
        self.__value = value
        self.__defaultvalue = defaultvalue

    def getname(self):
        return self.__name

    def gettitle(self):
        return self.__title

    def getvalue(self):
        return self.__value

    def getdefaultvalue(self):
        return self.__defaultvalue


class VariationGroup:

    def __init__(self, name, title, datalocation):
        self.__name = name
        self.__title = title
        self.__datalocation = datalocation
        self.__variations = []
        self.__triggers = []
        self.__jetradii = []
        self.__repo = ""
    
    def getname(self):
        return self.__name

    def setrepo(self, repo):
        self.__repo = repo

    def addvariation(self, variation):
        self.__variations.append(variation)

    def setjetradii(self, jetradii):
        self.__jetradii = jetradii

    def settriggers(self, triggers):
        self.__triggers = triggers

    def runcomp(self):
        if not len(self.__repo):
            logging.error("Source code repository not set - group %s not evaluated", self.__name)
        logging.info("Running group: %s", self.__name)
        for var in self.__variations:
            logging.info("Running variation: %s", var.getname());
            for trg in self.__triggers:
                logging.info("Running trigger: %s", trg)
                for r in self.__jetradii:
                    logging.info("Running jet Radius: %.1f", float(r)/10.)
                    unfoldedname = "JetSubstructureTree_FullJets_R%02d_%s_unfolded_zg.root" %(r, trg)
                    defaultfile = os.path.join(self.__datalocation, unfoldedname)
                    varname = "%s_%s" %(self.__name, var.getname())
                    vartitle = "%s, %s" %(self.__title, var.gettitle())
                    logging.info("default file: %s", defaultfile)
                    cmd="root -l -b -q \'%s(\"%s\", \"%s\", \"%s\", %d, %d)\'" %(os.path.join(self.__repo, "testRegularizationZg.cpp"), varname, vartitle, defaultfile, var.getdefaultvalue(), var.getvalue())
                    subprocess.call(cmd, shell=True)

def sortPlots(triggers, jetradii):
    formats = ["png", "pdf", "eps", "gif", "jpg"]
    basedir = os.getcwd()
    allfiles = os.listdir(basedir)
    for trg in triggers:
        for rad in jetradii:
            for form in formats:
                resultdir = os.path.join(basedir, trg, "R%02d" %rad, form)
                if not os.path.exists(resultdir):
                    os.makedirs(resultdir, 0755)
                tag = "R%02d_%s.%s" %(rad, trg, form)
                for fl in [x for x in allfiles if tag in x]:
                    shutil.move(os.path.join(basedir, fl), os.path.join(resultdir, fl))


def creadeRegulatizationGroup(datalocation):
    resultgroup = VariationGroup("regularization", "Regularization", datalocation)
    resultgroup.addvariation(Variation("lower", "Lower", 3, 4))
    resultgroup.addvariation(Variation("upper", "Upper", 3, 4))
    return resultgroup

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog = "steerTestVariation.py", description="steer evaluation of systematics")
    parser.add_argument("datalocation", metavar="DATALOCATION", type=str, help="location of the default output")
    arguments = parser.parse_args()
    repo = os.path.abspath(os.path.dirname(sys.argv[0]))
    logging.basicConfig(format='[%(levelname)s]: %(message)s', level=logging.INFO)
    triggers = ["INT7", "EJ1", "EJ2"]
    jetradii = [x for x in range(2,6)]
    
    group = creadeRegulatizationGroup(arguments.datalocation)
    group.setjetradii(jetradii)
    group.settriggers(triggers)
    group.setrepo(repo)

    basedir = os.getcwd()
    groupdir = os.path.join(basedir, group.getname())
    if not os.path.exists(groupdir):
        os.makedirs(groupdir, 0755)
    os.chdir(groupdir)
    group.runcomp()
    sortPlots(triggers, jetradii)
    os.chdir(basedir)