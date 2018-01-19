#! /usr/bin/env python

import sys, os, commands, zipfile, logging, getopt, md5

def GetMD5(localfile):
    m = md5()
    with open(localfile, "rb") as f:
        data = f.read() #read file in chunk and call update on each chunk if file is large.
        m.update(data)
    return m.hexdigest()

def aliencopy(sourcelocation, targetlocation):
    logging.info("Copy %s to %s", sourcelocation, targetlocation)
    if os.path.exists(targetlocation):
        remotemd5 = int(commands.getstatusoutput("gbbox md5sum %s | cut -f1" %sourcelocation)[1], 16)
        localmd5 = GetMD5(targetlocation)
        if localmd5 == remotemd5:
            logging.info("Not copying %s because it has been found uncorrupted locally" %sourcelocation)
    os.system("alien_cp alien://%s %s" %(sourcelocation, targetlocation))

def alienlist(inputdir):
    logging.debug("Scanning directory: %s", inputdir)
    content = []
    tmplist = commands.getstatusoutput("alien_ls %s" %(inputdir))[1].split("\n")
    for r in tmplist:
        content.append(r.lstrip().rstrip())
    return content

def extractZipfile(filename):
    if not ".zip" in filename or not os.path.exists(filename):
        return
    cwd = os.getcwd()
    os.chdir(os.path.dirname(filename))
    #unzip
    myarchive = zipfile.ZipFile(os.path.basename(filename))
    myarchive.extractall()
    os.chdir(cwd)

def TransferFile(inputfile, outputfile):
    if not os.path.exists(os.path.dirname(outputfile)):
        os.makedirs(os.path.dirname(outputfile), 0755)
    aliencopy(inputfile, outputfile)
    extractZipfile(outputfile)

def transfer(sample, trainrun, outputlocation, targetfile):
    run = 0
    ptharbin = 0
    for levelone in alienlist(sample):
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
        tmppath = os.path.join(sample, levelone)
        logging.info("Doing %s %s", "run" if isrun else "pthard-bin", levelone)
        for leveltwo in alienlist(tmppath):
            if not leveltwo.isdigit():
                continue
            if isrun:
                pthardbin = int(leveltwo)
            else:
                run = int(leveltwo)
            logging.info("Doing %s %s", "pthard-bin" if isrun else "run", leveltwo)
            tmppathtwo = os.path.join(tmppath, leveltwo)
            inputdir= os.path.join(tmppathtwo, trainrun)
            inputfile = os.path.join(inputdir, targetfile)
            outputdir = os.path.join(os.path.abspath(outputlocation), "%02d" %pthardbin, "%d" %run)
            outputfile = os.path.join(outputdir, targetfile)
            if not os.path.exists(outputfile):
                TransferFile(inputfile, outputfile)

def usage():
    print "Usage: ./copyFromGrid.py SAMPLE TRAINRUN OUTPUTPATH [OPTIONS]"
    print ""
    print "Arguments:"
    print "  SAMPLE:     Path in alien to the sample base directory"
    print "  TRAINRUN:   Full name of the train run (i. e. PWGJE/Jets_EMC_pp_MC/xxxx)"
    print "  OUTPUTPATH: Local directory where to write the output to"
    print ""
    print "Options:"
    print "  -f/--file=: Name of the file to be transferred (default: root_archive.zip)"
    print "  -d/--debug: Run with increased debug level"

if __name__ == "__main__":
    if len(sys.argv) < 4:
        usage()
        sys.exit(1)
    sample = sys.argv[1]
    trainrun = sys.argv[2]
    outputpath = str(sys.argv[3])
    outputfile = "root_archive.zip"
    debugging = False
    try:
        opt, arg = getopt.getopt(sys.argv[4:], "f:d", ["file=", "debug"])
        for o, a in opt:
            if o in ("-d", "--debug"):
                debugging = True
            if o in ("-f", "--file"):
                outputfile = a
    except getopt.getopterror as e:
        sys.exit(1)
    loglevel=logging.INFO
    if debugging:
        loglevel = logging.DEBUG
    logging.basicConfig(format='[%(levelname)s]: %(message)s', level=loglevel)
    transfer(sample, trainrun, outputpath, outputfile)
