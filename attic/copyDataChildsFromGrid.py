#! /usr/bin/env python

import logging
import os
import sys
from commands import getstatusoutput as ospipe

class AlienException(Exception):

    def __init__(self, command, errorcode):
        super(AlienException, self).__init__()
        self.__command = command
        self.__errorcode = errorcode

    def __str__(self):
        return "command %s exited with error code %d" %(self.__command, self.__errorcode)

    def get_errorcode(self):
        return self.__errorcode

    def get_command(self):
        return self.__command

def alien_ls(inputdir):
    alienresult = ospipe("alien_ls %s" %inputdir)
    if alienresult[0] > 0:
        raise AlienException("alien_ls", alienresult[1])
    tmp = alienresult[1].split("\n")
    return [cont.rstrip().lstrip() for cont in tmp]

def alien_cp(inputfile, outputfile):
    status = os.system("alien_cp %s %s" %(inputfile, outputfile))
    if status > 0:
        raise AlienException("alien_cp", status)

def find_childs(trainrun):
    trainname = trainrun[0:trainrun.rfind("/")]
    trainruntoken = trainrun[trainrun.rfind("/")+1:]
    traindir = "/alice/cern.ch/user/a/alitrain/%s" %trainname
    replacetag = "%s_" %trainruntoken
    return [child.replace(replacetag, "") for child in alien_ls(traindir) if "child" in child and trainruntoken in child]

def do_transfer(trainrun, outputdir, mergedir, rootfilename):
    if not os.path.exists(outputdir):
        os.makedirs(outputdir, 0755)
    try:
        traindir = "/alice/cern.ch/user/a/alitrain/%s" %(trainrun[0:trainrun.rfind("/")])
        for child in find_childs(trainrun):
            childtag = "%s_%s" %(trainrun[trainrun.rfind("/")+1:], child)
            print childtag
            childdir = os.path.join(outputdir, child)
            if not os.path.exists(childdir):
                os.makedirs(childdir, 0755)
            inputfile = os.path.join(traindir, childtag, mergedir, rootfilename)
            outputfile = os.path.join(childdir, rootfilename)
            logging.info("Copy %s to %s", inputfile, outputfile)
            try:
                alien_cp("alien://%s" %inputfile, outputfile)
            except AlienException as ec:
                logging.error("Failed copy file from grid: %s", ec)
    except AlienException as el:
        logging.error("Failed getting childs: %s", el)

if __name__ == "__main__":
    TRAINDIR = sys.argv[1]
    OUTPUTDIR = sys.argv[2]
    MERGEDIR = sys.argv[3]
    ROOTFILE = sys.argv[4] if len(sys.argv) > 4 else "root_archive.zip"
    LOGLEVEL = logging.INFO
    logging.basicConfig(format='[%(levelname)s]: %(message)s', level=LOGLEVEL)
    do_transfer(TRAINDIR, OUTPUTDIR, MERGEDIR, ROOTFILE)
