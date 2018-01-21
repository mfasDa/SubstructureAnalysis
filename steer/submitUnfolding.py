#! /usr/bin/env python

import os
import sys

def make_jobscript(jobscript, sourcedir, workdir, unfoldingmacro, datafile, mcfile):
    if not os.path.exists(workdir):
        os.makedirs(workdir, 0755)
    writer = open(jobscript, 'w')
    writer.write("#! /bin/bash\n")
    writer.write("#PBS -o %s/stdout\n" %workdir)
    writer.write("#PBS -e %s/stderr\n" %workdir)
    writer.write("\n")
    writer.write("cd %s\n" %workdir)
    writer.write("%s/runUnfolding.sh %s %s %s %s\n" %(sourcedir, unfoldingmacro, workdir, datafile, mcfile))
    writer.write("rm %s\n" %jobscript)
    os.chmod(jobscript, 0755)
    return jobscript

def submit_job(jobscript):
    os.system("qsub %s" %jobscript)

def GetUnfoldingDir(sourcedir):
    return sourcedir.replace("steer", "unfolding")

def submit_unfolding(datadir, mcdir, jetradius, trigger, observable):
    basedir = os.path.abspath(sys.argv[0])
    unfoldingdir = GetUnfoldingDir(basedir)
    unfoldingscript = os.path.join(unfoldingdir, "RunUnfolding%s.cpp" %observable)
    datafile = os.path.join(os.path.abspath(datadir), "JetSubstructureTree_R%02d_%s.root" %(int(jetradius * 10.), trigger))
    mcfile = os.path.join(os.path.abspath(mcdir), "JetSubstructure_R%02d_INT7_merged.root" %(int(jetradius * 10.)))
    outputdir = os.path.join(os.getcwd(), "Unfolded_%s_R%02d_%s" %(observable, jetradius, trigger))
    jobscript = os.path.join(outputdir, "jobscript.sh")
    submit_job(make_jobscript(jobscript, basedir, outputdir, unfoldingscript, datafile, mcfile))

if __name__ == "__main__":
    submit_unfolding(str(sys.argv[1]), str(sys.argv[2]), float(sys.argv[3]), str(sys.argv[4]), str(sys.argv[5]))
