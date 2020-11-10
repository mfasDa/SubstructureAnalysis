#! /usr/bin/env python3
import argparse
import os
import sys
import subprocess

trainbasedir = "/alice/cern.ch/user/a/alitrain/PWGZZ/MCGen_PbPb"

def listGridDir(inputdir):
    status, stdout = subprocess.getstatusoutput("alien_ls {INPUTDIR}".format(INPUTDIR=inputdir))
    content = []
    for cnt in stdout.split("\n"):
        content.append(cnt.replace("/", ""))
    return content

def hasFile(inputdir, filename):
    return filename in listGridDir(inputdir)

def copyFile(gridfile, outputfile):
    subprocess.call("alien_cp {GRIDFILE} file://{OUTPUTFILE}".format(GRIDFILE=gridfile, OUTPUTFILE=outputfile), shell=True)

def getPtHardBinForTrain(inputdir):
    status, stdout = subprocess.getstatusoutput("alien.py cat {INPUTDIR}/env.sh | grep PERIOD_NAME".format(INPUTDIR=inputdir))
    periodname = stdout[stdout.index("=")+1:].replace("'", "")
    return int(toks = periodname.split("_")[1].replace("pthard", ""))

def getTrainIDs(mintrain, maxtrain):
    dirs = listGridDir(trainbasedir)
    trains = {}
    for dr in dirs:
        trainid = int(dr[:dr.index("_")])
        if trainid >= mintrain and trainid <= maxtrain:
            trains[getPtHardBinForTrain(os.path.join(trainbasedir, dr))] = trainid
    return trains

def getTrainIDsFast():
    return {
        1:  "1813_20201104-2257",
        2:  "1814_20201104-2257",
        3:  "1815_20201104-2258",
        4:  "1816_20201104-2258",
        5:  "1817_20201104-2258",
        6:  "1818_20201104-2257",
        7:  "1819_20201104-2257",
        8:  "1820_20201104-2258",
        9:  "1821_20201104-2258",
        10:  "1822_20201104-2259",
        11:  "1823_20201105-0026",
        12:  "1824_20201105-0026",
        13:  "1825_20201105-0911",
        14:  "1826_20201105-0911",
        15:  "1827_20201105-0911",
        16:  "1828_20201105-0911",
        17:  "1829_20201105-0912",
        18:  "1830_20201105-0912",
        19:  "1831_20201105-0912",
        20:  "1832_20201105-0913"
    }

def download_stage(griddir, outputdir, filename):
    for chunk in listGridDir(griddir):
        chunkinfile = os.path.join(griddir, chunk, filename)
        chunkoutfile = os.path.join(outputdir, chunk, filename)
        chunkdir = os.path.dirname(chunkoutfile)
        if not os.path.exists(chunkdir):
            os.makedirs(chunkdir, 0o755)
        copyFile(chunkinfile, chunkoutfile)

if __name__ == "__main__":
    parser = argparse.ArgumentParser("copyMCGenGromGrid", "copy MC output from grid")
    parser.add_argument("mintrain", metavar="MINTRAIN", type=int, help="Min. train ID")
    parser.add_argument("maxtrain", metavar="MAXTRAIN", type=int, help="Max. train ID")
    parser.add_argument("-o", "--outputdir", metavar="OUTPUTDIR", type=str, default=os.getcwd(), help="Output directory")
    parser.add_argument("-f", "--filename", metavar="FILENAME", type=str, default="AnalysisResults.root", help="File to copy")
    parser.add_argument("-t", "--testmode", metavar="TESTMODE", type=bool, action="store_true", help="Run test mode on existing dataset")
    args = parser.parse_args()
    trainruns=None
    if args.testmode:
        trainruns = getTrainIDsFast()
    else:
        trainruns = getTrainIDs(args.mintrain, args.maxtrain)
    for pthardbin,trainpath in trainruns.items():
        print("Using train {TRAINRUN} for pt-hard bin {PTHARDBIN} ...".format(TRAINRUN=trainpath, PTHARDBIN=pthardbin))
        gridpath = os.path.join(trainbasedir, trainpath)
        outputpath = os.path.join(args.outputdir, "%02d" %pthardbin)
        if not os.path.exists(outputpath):
            os.makedirs(outputpath, 0o755)
        mergeddir = os.path.join(gridpath, "merge")
        if hasFile(mergeddir, args.filename):
            mergedfile = os.path.join(mergeddir, args.filename)
            targetfile = os.path.join(outputpath, args.filename)
            print("Found file {MERGEDFILE} for pt-hard bin {PTHARDBIN} ...".format(MERGEDFILE=mergedfile, PTHARDBIN=pthardbin))
            copyFile(mergedfile, targetfile)
        else:
            print("No merged file found in {GRIDDIR}, copying stage 1 ...".format(GRIDDIR=gridpath))
            download_stage(os.path.join(gridpath, "1", "Stage_1"), outputpath, args.filename)