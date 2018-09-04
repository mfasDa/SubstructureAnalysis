#! /usr/bin/env python
import argparse
import logging
import os
import shutil
import sys
import subprocess

class Variation:

    def __init__(self, name, title, datalocation):
        self.__name = name
        self.__title = title
        self.__datalocation = datalocation 

    def getname(self):
        return self.__name

    def gettitle(self):
        return self.__title

    def getdatalocation(self):
        return self.__datalocation


class VariationGroup:

    def __init__(self, name, title, basedir, defaultlocation):
        self.__name = name
        self.__title = title
        self.__basedir = basedir
        self.__variations = []
        self.__triggers = []
        self.__jetradii = []
        self.__defaultlocation = defaultlocation
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
            datadir = os.path.join(self.__basedir, var.getdatalocation())
            for trg in self.__triggers:
                logging.info("Running trigger: %s", trg)
                for r in self.__jetradii:
                    logging.info("Running jet Radius: %.1f", float(r)/10.)
                    unfoldedname = "JetSubstructureTree_FullJets_R%02d_%s_unfolded_zg.root" %(r, trg)
                    defaultfile = os.path.join(self.__defaultlocation, unfoldedname)
                    varfile = os.path.join(datadir, unfoldedname) 
                    varname = "%s_%s" %(self.__name, var.getname())
                    vartitle = "%s, %s" %(self.__title, var.gettitle())
                    logging.info("default file: %s, variation file: %s", defaultfile, varfile)
                    cmd="root -l -b -q \'%s(\"%s\", \"%s\", \"%s\", \"%s\")\'" %(os.path.join(self.__repo, "testVariationZg.cpp"), varname, vartitle, defaultfile, varfile)
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


def creadeTruncationGroup(basedir, defaultlocation):
    resultgroup = VariationGroup("truncation", "Truncation", os.path.join(basedir, "truncation"), defaultlocation)
    resultgroup.addvariation(Variation("loose", "Loose", "loose"))
    resultgroup.addvariation(Variation("strong", "Strong", "strong"))
    return resultgroup

def createBinningGroup(basedir, defaultlocation):
    resultgroup = VariationGroup("binning", "Binning", os.path.join(basedir, "binning"), defaultlocation)
    for o in range(1,5):
        resultgroup.addvariation(Variation("option%d" %o, "Option %d" %o, "option%d" %o))
    return resultgroup

def createPriorsGroup(basedir, defaultlocation):
    resultgroup = VariationGroup("priors", "Priors", os.path.join(basedir, "priors"), defaultlocation)
    resultgroup.addvariation(Variation("weighted", "weighted", "default"))
    return resultgroup

def createClusterTimeGroup(basedir, defaultlocation):
    resultgroup = VariationGroup("emcaltimecut", "EMCAL time cut", basedir, defaultlocation)
    resultgroup.addvariation(Variation("loose", "Loose", "20180823_emcalloosetimecut/unfolded_zg_sys"))
    resultgroup.addvariation(Variation("strong", "Strong", "20180823_emcalstrongtimecut/unfolded_zg_sys"))
    return resultgroup

def createTrackingEffGroup(basedir, defaultlocation):
    resultgroup = VariationGroup("trackingeff", "Tracking efficiency", basedir, defaultlocation)
    resultgroup.addvariation(Variation("strong", "Strong", "20180823_trackingeff/unfolded_zg_sys"))
    return resultgroup

def createSeedingGroup(basedir, defaultlocation):
    resultgroup = VariationGroup("seeding", "Seedig", basedir, defaultlocation)
    resultgroup.addvariation(Variation("strong", "Strong", "20180813_emchighthresh/unfolded_zg_sys"))
    return resultgroup

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog = "steerTestVariation.py", description="steer evaluation of systematics")
    parser.add_argument("defaultlocation", metavar="DEFAULTLOCATION", type=str, help="location of the default output")
    parser.add_argument("unfoldsyslocation", metavar="UNFOLDINGSYSLOCATION", type=str, help="location of the unfolding systematics output")
    parser.add_argument("detectorsyslocation", metavar="DETECTORSYSLOCATION", type=str, help="location of the detector systematics output")
    arguments = parser.parse_args()
    repo = os.path.abspath(os.path.dirname(sys.argv[0]))
    logging.basicConfig(format='[%(levelname)s]: %(message)s', level=logging.INFO)
    triggers = ["INT7", "EJ1", "EJ2"]
    jetradii = [x for x in range(2,6)]
   # producers_unfolding = [creadeTruncationGroup, createBinningGroup, createPriorsGroup]
    #producers_detector = [createTrackingEffGroup, createClusterTimeGroup, createSeedingGroup]
    producers_unfolding = []
    producers_detector = [createSeedingGroup]
    taskgroups = []
    for prod in producers_unfolding:
        group = prod(arguments.unfoldsyslocation, arguments.defaultlocation)
        group.setjetradii(jetradii)
        group.settriggers(triggers)
        group.setrepo(repo)
        taskgroups.append(group)
    for prod in producers_detector:
        group = prod(arguments.detectorsyslocation, arguments.defaultlocation)
        group.setjetradii(jetradii)
        group.settriggers(triggers)
        group.setrepo(repo)
        taskgroups.append(group)
    basedir = os.getcwd()
    for group in taskgroups:
        groupdir = os.path.join(basedir, group.getname())
        if not os.path.exists(groupdir):
            os.makedirs(groupdir, 0755)
        os.chdir(groupdir)
        group.runcomp()
        sortPlots(triggers, jetradii)
        os.chdir(basedir)
