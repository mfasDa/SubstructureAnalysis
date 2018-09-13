#! /usr/bin/env python
import argparse
import logging
import os
import shutil
import sys
import subprocess

class Variation:

    def __init__(self, name, title, unfoldingmethod, regularization, datalocation):
        self.__name = name
        self.__title = title
        self.__unfoldingmethod = unfoldingmethod
        self.__regularization = regularization
        self.__datalocation = datalocation 

    def getname(self):
        return self.__name

    def gettitle(self):
        return self.__title

    def getdatalocation(self):
        return self.__datalocation

    def getunfoldingmethod(self):
        return self.__unfoldingmethod

    def getregularization(self):
        return self.__regularization


class VariationGroup:

    def __init__(self, name, title, basedir):
        self.__name = name
        self.__title = title
        self.__basedir = basedir
        self.__variations = []
        self.__jetradii = []
        self.__defaultsettings = None
        self.__repo = ""
    
    def getname(self):
        return self.__name

    def setrepo(self, repo):
        self.__repo = repo

    def setdefaults(self, defaults):
        self.__defaultsettings = defaults

    def addvariation(self, variation):
        self.__variations.append(variation)

    def setjetradii(self, jetradii):
        self.__jetradii = jetradii

    def runcomp(self):
        if not len(self.__repo):
            logging.error("Source code repository not set - group %s not evaluated", self.__name)
        logging.info("Running group: %s", self.__name)
        for var in self.__variations:
            logging.info("Running variation: %s", var.getname());
            datadir = os.path.join(self.__basedir, var.getdatalocation())
            for r in self.__jetradii:
                logging.info("Running jet Radius: %.1f", float(r)/10.)
                rootfiledefault = "corrected1D%s_R%02d.root" %(self.__defaultsettings.getunfoldingmethod(), r)
                rootfilevar = "corrected1D%s_R%02d.root" %(var.getunfoldingmethod(), r)
                defaultfile = os.path.join(self.__defaultsettings.getdatalocation(), rootfiledefault)
                varfile = os.path.join(datadir, rootfilevar) 
                varname = "%s_%s" %(self.__name, var.getname())
                vartitle = "%s, %s" %(self.__title, var.gettitle())
                logging.info("default file: %s, variation file: %s, methods: %s / %s regularizations: %d / %d", defaultfile, varfile, self.__defaultsettings.getunfoldingmethod(), var.getunfoldingmethod(), self.__defaultsettings.getregularization(), var.getregularization())
                cmd="root -l -b -q \'%s(\"%s\", \"%s\", \"%s\", \"%s\", %d, %d)\'" %(os.path.join(self.__repo, "testVariation1D.cpp"), varname, vartitle, defaultfile, varfile, self.__defaultsettings.getregularization(), var.getregularization())
                subprocess.call(cmd, shell=True)

def sortPlots(jetradii):
    formats = ["png", "pdf", "eps", "gif", "jpg"]
    basedir = os.getcwd()
    allfiles = os.listdir(basedir)
    for rad in jetradii:
        for form in formats:
            resultdir = os.path.join(basedir, "R%02d" %rad, form)
            if not os.path.exists(resultdir):
                os.makedirs(resultdir, 0755)
            tag = "R%02d.%s" %(rad, form)
            for fl in [x for x in allfiles if tag in x]:
                shutil.move(os.path.join(basedir, fl), os.path.join(resultdir, fl))


def creadeTruncationGroup(basedir):
    resultgroup = VariationGroup("truncation", "Truncation", os.path.join(basedir, "truncation"))
    resultgroup.addvariation(Variation("loose", "Loose", "Bayes", 4, "loose"))
    resultgroup.addvariation(Variation("strong", "Strong", "Bayes", 4 , "strong"))
    return resultgroup

def createBinningGroup(basedir):
    resultgroup = VariationGroup("binning", "Binning", os.path.join(basedir, "binning"))
    for o in range(1,5):
        resultgroup.addvariation(Variation("option%d" %o, "Option %d" %o, "Bayes", 4, "option%d" %o))
    return resultgroup

def createPriorsGroup(basedir):
    resultgroup = VariationGroup("priors", "Priors", os.path.join(basedir, "priors"))
    resultgroup.addvariation(Variation("weighted", "weighted", "Bayes", 4, "default"))
    return resultgroup

def createClusterTimeGroup(basedir):
    resultgroup = VariationGroup("emcaltimecut", "EMCAL time cut", basedir)
    resultgroup.addvariation(Variation("loose", "Loose", "Bayes", 4, "emcaltimeloose"))
    resultgroup.addvariation(Variation("strong", "Strong", "Bayes", 4, "emcaltimestrong"))
    return resultgroup

def createRegularizationGroup(basedir):
    resultgroup = VariationGroup("regularization", "Regularization", basedir)
    resultgroup.addvariation(Variation("low", "Low", "Bayes", 3, "default"))
    resultgroup.addvariation(Variation("high", "High", "Bayes", 5, "default"))
    return resultgroup

def createUnfoldingMethodGroup(basedir):
    resultgroup = VariationGroup("unfoldingmethod", "Unfolding method", basedir)
    resultgroup.addvariation(Variation("svd", "SVD", "SVD", 4, "default"))
    return resultgroup

def createTrackingEffGroup(basedir):
    resultgroup = VariationGroup("trackingeff", "Tracking efficiency", basedir)
    resultgroup.addvariation(Variation("strong", "Strong", "Bayes", 4, "trackingeff"))
    return resultgroup

def createClusterizerGroup(basedir):
    resultgroup = VariationGroup("clusterizerAlgorithm", "Clusterizer algorithm", basedir)
    resultgroup.addvariation(Variation("clusterizerV1", "ClusterizerV1", "Bayes", 4, "emcalclusterizerv1"))
    return resultgroup

def createHadCorrGroup(basedir):
    resultgroup = VariationGroup("hadronicCorrection", "Hadronic correction", basedir)
    resultgroup.addvariation(Variation("loose", "Loose", "Bayes", 4, "emcalhadcorrf0"))
    return resultgroup

def createTriggerEffGroup(basedir):
    resultgroup = VariationGroup("triggereff", "Trigger efficiency", os.path.join(basedir, "triggereff"))
    resultgroup.addvariation(Variation("loose", "Loose", "Bayes", 4, "loose"))
    resultgroup.addvariation(Variation("strong", "Strong", "Bayes", 4, "strong"))
    return resultgroup

def createSeedingGroup(basedir):
    resultgroup = VariationGroup("seeding", "Seedig", basedir)
    resultgroup.addvariation(Variation("strong", "Strong", "Bayes", 4, "emcalseed"))
    return resultgroup

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog = "steerTestVariation.py", description="steer evaluation of systematics")
    parser.add_argument("defaultlocation", metavar="DEFAULTLOCATION", type=str, help="location of the default output")
    parser.add_argument("syslocation", metavar="SYSLOCATION", type=str, help="location of the unfolding systematics output")
    arguments = parser.parse_args()
    repo = os.path.abspath(os.path.dirname(sys.argv[0]))
    logging.basicConfig(format='[%(levelname)s]: %(message)s', level=logging.INFO)
    jetradii = [x for x in range(2,6)]
    #producers_unfolding = [creadeTruncationGroup, createBinningGroup, createPriorsGroup, createRegularizationGroup, 
    #                       createUnfoldingMethodGroup, createTrackingEffGroup, createClusterTimeGroup, 
    #                       createSeedingGroup, createTriggerEffGroup, createClusterizerGroup, createHadCorrGroup]

    defaultsettings = Variation("default", "Reference", "Bayes", 4, arguments.defaultlocation)
    #producers_unfolding = []
    producers_unfolding = [createTriggerEffGroup]
    taskgroups = []
    for prod in producers_unfolding:
        group = prod(arguments.syslocation)
        group.setjetradii(jetradii)
        group.setrepo(repo)
        group.setdefaults(defaultsettings)
        taskgroups.append(group)
    basedir = os.getcwd()
    for group in taskgroups:
        groupdir = os.path.join(basedir, group.getname())
        if not os.path.exists(groupdir):
            os.makedirs(groupdir, 0755)
        os.chdir(groupdir)
        group.runcomp()
        sortPlots(jetradii)
        os.chdir(basedir)
