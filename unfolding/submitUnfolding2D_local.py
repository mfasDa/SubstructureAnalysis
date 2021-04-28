#! /usr/bin/env python3

import argparse
import os
import subprocess
import sys

scriptname = os.path.dirname(os.path.abspath(sys.argv[0]))
repository = os.path.dirname(scriptname)

def create_jobscript(workdir, observable, radius, datafile, mcfile, effcorr, binvar, partition):
    if not os.path.exists(workdir):
        os.makedirs(workdir, 0o755)
    rstring = "R%02d" %radius
    jobscriptname = os.path.join(workdir, "jobscript_{}_{}.sh".format(observable, rstring))
    logfile = os.path.join(workdir, "unfolding_{}_{}.log".format(observable, rstring))
    jobname = "corr{}_{}".format(rstring, observable)
    unfoldingmacro = os.path.join(repository, "unfolding", "runUnfolding2D_FromFile.C")
    effcorrstring = "kTRUE" if effcorr > 0 else "kFALSE"
    with open(jobscriptname, "w") as jobscriptwriter:
        jobscriptwriter.write("#! /bin/bash\n")
        jobscriptwriter.write("#SBATCH -n 1\n")
        jobscriptwriter.write("#SBATCH -p {}\n".format(partition))
        jobscriptwriter.write("#SBATCH -J {}\n".format(jobname))
        jobscriptwriter.write("#SBATCH -o {}\n".format(logfile))
        jobscriptwriter.write("\n")
        jobscriptwriter.write("export ALIBUILD_WORK_DIR=/software/markus/alice/sw\n")
        jobscriptwriter.write("eval `alienv --no-refresh printenv AliPhysics/latest-aliceo2-o2`\n")
        jobscriptwriter.write("cd {}\n".format(workdir))
        jobscriptwriter.write("root -l -b -q \'{}(\"{}\", \"{}\", \"{}\", \"{}\", \"{}\", {})\'\n".format(unfoldingmacro, datafile, mcfile, observable, binvar, rstring, effcorrstring))
        jobscriptwriter.write("echo \"Done ...\"\n")
        jobscriptwriter.write("rm -vf {}\n".format(jobscriptname))
        jobscriptwriter.close()
    return jobscriptname

if __name__ == "__main__":
    print("Using repository   {}".format(repository))
    parser = argparse.ArgumentParser("submitUnfolding2D_local.py", "Submitter for 2D unfolding on the 587 cluster")
    parser.add_argument("workdir", metavar="WORKDIR", type=str, help="Output location")
    parser.add_argument("datafile", metavar="DATAFILE", type=str, help="Data file")
    parser.add_argument("mcfile", metavar="MCFILE", type=str, help="File with response matrix")
    parser.add_argument("-b", "--binvar", metavar="BINVAR", type=str, default="Default", help="Binning variation (default: default)")
    parser.add_argument("-o", "--observables", metavar="OBSERVABLES", type=str, default="all", help="Observables to unfold, comma-separated (default: all - all observables)")
    parser.add_argument("-p", "--partition", metavar="PARTITION", type=str, default="long", help="partition to submit to (default: long)")
    parser.add_argument("-e", "--effcorr", metavar="EFFCOR", type=int, default=1, help="Efficiency and purity correction (1 - on, 0 - off)")
    args = parser.parse_args()
    print("Using WORKDIR:     {}".format(args.workdir))
    print("Using DATAFILE     {}".format(args.datafile))
    print("Using MCFILE       {}".format(args.mcfile))
    observablesAll = ["Zg", "Rg", "Nsd", "Thetag"]
    observables = []
    if args.observables == "all":
        print("Unfolding for all observables")
        observables = observablesAll
    else:
        observablesSelected = args.observables.replace(" ", "").split(",")
        for obs in observablesSelected:
            if obs in observablesAll:
                print("Enabled unfolding for observable {OBSERVABLE}".format(OBSERVABLE=obs))
                observables.append(obs)
    for observable in observables:
        for r in range(2, 7):
            subprocess.call(["sbatch",  create_jobscript(args.workdir, observable, r, args.datafile, args.mcfile, args.effcorr, args.binvar, args.partition)])