#! /usr/bin/env python

import commands
import getopt
import logging
import md5
import os
import sys

def alien_ls(inputpath):
    dircontent = commands.getstatusoutput("alien_ls %s" %inputpath)[1]
    directories = dircontent.split("\n")
    return directories

def alien_cp(inputfile, outputfile):
    os.system("alien_cp alien://%s %s" %(inputfile, outputfile))

def getchildpaths(inputdir, trainid):
    childdirs = filter(lambda n : n.split("_")[0] == trainid and "child" in n, alien_ls(inputdir))
    return map(lambda c : os.path.join(inputdir, c), childdirs)

def getchildnumber(inputpath):
    if not "child" in inputpath:
        return 0
    return int(os.path.basename(inputpath).split("_")[3])

def getmd5local(localfile):
    md5sum = md5.md5()
    with open(localfile, "rb") as filereader:
        data = filereader.read() #read file in chunk and call update on each chunk if file is large.
        md5sum.update(data)
    return md5sum.hexdigest()

def getmd5alien(alienfile):
    return int(commands.getoutput("gbbox md5sum %s | cut -f1" %alienfile)[1], 16)

def checkconsistency(alienfile, localfile):
    return getmd5alien(alienfile) == getmd5local(localfile)

def main(outputpath, trainname, trainid, filestocopy):
    outfilenames = filestocopy.split(",")
    traindir = os.path.join("/alice/cern.ch/user/a/alitrain", trainname)
    
    if not os.path.exists(outputpath):
        os.makedirs(outputpath)

    for childpath in getchildpaths(traindir, trainid):
        child_id = getchildnumber(childpath)
        logging.info("Processing child %d", child_id)

        locchilddir = os.path.join(outputpath, "child_%d" %child_id)
        if not os.path.exists(locchilddir):
            os.makedirs(locchilddir, 0755)

        for transferfile in outfilenames:
            alienfile = os.path.join(childpath, "merge", transferfile)
            localfile = os.path.join(locchilddir, transferfile)
            if os.path.exists(localfile) and checkconsistency(alienfile, localfile):
                logging.info("Not copying %s because it is already found locally", alienfile)
                continue
            logging.info("Copying %s to %s", alienfile, localfile);
            alien_cp(alienfile, localfile)

def usage():
    print "Usage:"
    print ""
    print "./copyFromGridMeta.py OUTPUTPATH TRAINNAME TRAINID FILESTOCOPY [OPTIONS]"
    print ""
    print "Arguments:"
    print "  OUTPUTPATH:  Path where to store the files locally"
    print "  TRAINNAME:   Name of the lego train (i.e. PWGJE/Jets_EMC_pp)"
    print "  TRAINID:     ID of the train run (only ID number needed, no full tag)"
    print "  FILESTOCOPY: Comma-separated list of files to be copied"
    print ""
    print "Options:"
    print "  -d/--debug:   Add debug logging"
    print ""
    print "After copy the files will be arraged as"
    print ""
    print "  OUTPUTPATH/child_x/file1"
    print "  OUTPUTPATH/child_x/file2"
    print "  ..."

if __name__ == "__main__":
    if len(sys.argv) < 5:
        usage()
        sys.exit(1)
    OUTPUTDIR   = sys.argv[1]
    TRAINNAME   = sys.argv[2]
    TRAINID     = sys.argv[3]
    FILESTOCOPY = sys.argv[4]
    debugging = False
    try:
        opt, arg = getopt.getopt(sys.argv[4:], "d", ["debug"])
        for o, a in opt:
            if o in ("-d", "--debug"):
                debugging = True
    except getopt.getopterror as e:
        sys.exit(1)
    loglevel=logging.INFO
    if debugging:
        loglevel = logging.DEBUG
    logging.basicConfig(format='[%(levelname)s]: %(message)s', level=loglevel)
    main(OUTPUTDIR, TRAINNAME, TRAINID, FILESTOCOPY)