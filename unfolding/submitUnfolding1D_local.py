#! /usr/bin/env python3

import argparse
import os
import sys
import subprocess

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitUnfolding1D_local", "Submitter for 1D unfolding")
    parser.add_argument("datafile", metavar="DATFILE", type=str, required=True, help="Data input")
    parser.add_argument("mcfile", metavar="MCFILE", type=str, required=True, help="MC input")
    parser.add_argument("-w", "--workdir", metavar="WORKDIR", type=str, default=os.getcwd(), help="Working directory")
    parser.add_argument("-s", "--sysvar", metavar="SYSVAR", type=str, default="tc200", help="Systematics variation")
    parser.add_argument("-m", "--macro", metavar="MACRO", type=str, default="runCorrectionChain1DSVD_SpectrumTaskSimplePoor.sh", help="Unfolding macro")
    parser.add_argument("-j", "--jobtag", metavar="JOBTAG", type=str, default="corr1D", help="Job tag in jobname")
    args = parser.parse_args()

    executable = os.path.join(os.path.abspath(os.path.dirname(sys.argv[0])), "runUnfolding1D_local.sh")
    os.chdir(args.workdir)
    for radius in range(2, 7):
        logfile="joboutput_R%02d" %radius
        jobname="%s_R%02d" %(args.jobtag, radius)
        cmd = "sbatch -N 1 -n 1 --partition short -J {JNAME} -o {OUTFILE} {EXE} {WDIR} {DATAFILE} {MCFILE} {SYSVAR} {RADIUS} {MACRO}".format(JNAME=jobname, OUTFILE=logfile, EXE=executable, WDIR=args.workdir, DATAFILE=args.datafile, MCFILE=args.mcfile, SYSVAR=args.sysvar, RADIUS=radius, MACRO=args.macro)
        print("Running: %s" %cmd)
        subprocess.call(cmd, shell=True)