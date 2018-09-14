#! /usr/bin/env python
import os
import subprocess
import sys

def find_files(filename, years, basedir):
    outlist = []
    for rot, drs, fls in os.walk(basedir):
        for f in fls:
            if filename in f:
                tmppath = os.path.join(rot, f)
                print tmppath
                for year in years:
                    yearstring = "LHC%d" %year
                    # print "testing %s" %yearstring
                    if yearstring in tmppath:
                        outlist.append(tmppath)
                        # print "accepted %s" %yearstring
    return outlist

def merge_years(filename, years, basedir):
    for year in years:
        outdir = os.path.join(basedir, "merged_%d" %year)
        if not os.path.exists(outdir):
            os.makedirs(outdir, 0755)
        outfile = os.path.join(outdir, filename)
        print "Target %s" %outfile
        cmd = "hadd -f %s" %outfile
        for f in find_files(filename, [year], basedir):
            print "Adding %s" %f
            cmd += " %s" %f
        subprocess.call(cmd, shell=True)
    # merge 16+17 if required
    if 16 in years and 17 in years:
        outdir = os.path.join(basedir, "merged_1617")
        if not os.path.exists(outdir):
            os.makedirs(outdir, 0755)
        outfile = os.path.join(outdir, filename)
        print "Target %s" %outfile
        cmd = "hadd -f %s" %outfile
        for f in find_files(filename, [16, 17], basedir):
            print "Adding %s" %f
            cmd += " %s" %f
        subprocess.call(cmd, shell=True)

if __name__ == "__main__":
    jettype = sys.argv[1] if len(sys.argv) > 1 else "FullJets"
    basedir = os.getcwd()
    for trg in ["INT7", "EJ1", "EJ2"]:
        years = [17]
        if trg == "INT7":
            years.append(16)
        for r in range(2, 6):
            filename = "JetSubstructureTree_%s_R%02d_%s.root" %(jettype, r, trg)
            merge_years(filename, years, basedir)
    merge_years("AnalysisResults_split.root", [16, 17], basedir)