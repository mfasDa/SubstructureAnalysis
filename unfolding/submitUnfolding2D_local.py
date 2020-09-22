#! /usr/bin/env python3

import os
import subprocess
import sys

scriptname = os.path.dirname(os.path.abspath(sys.argv[0]))
repository = os.path.dirname(scriptname)

def create_jobscript(workdir, observable, radius, datafile, mcfile):
    if not os.path.exists(workdir):
        os.makedirs(workdir, 0o755)
    rstring = "R%02d" %radius
    jobscriptname = os.path.join(workdir, "jobscript_{}_{}.sh".format(observable, rstring))
    logfile = os.path.join(workdir, "unfolding_{}_{}.log".format(observable, rstring))
    jobname = "corr{}_{}".format(rstring, observable)
    unfoldingmacro = os.path.join(repository, "unfolding", "runUnfolding2D_FromFile.C")
    with open(jobscriptname, "w") as jobscriptwriter:
        jobscriptwriter.write("#! /bin/bash\n")
        jobscriptwriter.write("#SBATCH -n 1\n")
        jobscriptwriter.write("#SBATCH -p long\n")
        jobscriptwriter.write("#SBATCH -J {}\n".format(jobname))
        jobscriptwriter.write("#SBATCH -o {}\n".format(logfile))
        jobscriptwriter.write("\n")
        jobscriptwriter.write("export ALIBUILD_WORK_DIR=/software/markus/alice/sw\n")
        jobscriptwriter.write("eval `alienv --no-refresh printenv AliPhysics/latest-aliceo2-o2`\n")
        jobscriptwriter.write("cd {}\n".format(workdir))
        jobscriptwriter.write("root -l -b -q \'{}(\"{}\", \"{}\", \"{}\", \"{}\")\'\n".format(unfoldingmacro, datafile, mcfile, observable, rstring))
        jobscriptwriter.write("echo \"Done ...\"\n")
        jobscriptwriter.write("rm -vf {}\n".format(jobscriptname))
        jobscriptwriter.close()
    return jobscriptname

if __name__ == "__main__":
    print("Using repository   {}".format(repository))
    WORKDIR = sys.argv[1]
    DATAFILE = sys.argv[2]
    MCFILE = sys.argv[3]
    print("Using WORKDIR:     {}".format(WORKDIR))
    print("Using DATAFILE     {}".format(DATAFILE))
    print("Using MCFILE       {}".format(MCFILE))
    observables = ("Zg", "Rg", "Nsd", "Thetag")
    for observable in observables:
        for r in range(2, 7):
            subprocess.call(["sbatch",  create_jobscript(WORKDIR, observable, r, DATAFILE, MCFILE)])