#! /usr/bin/env python3

import argparse
import os
import subprocess

def submit(inputdir: str, outputdir: str, yeartag: str, partition: str = "long"):
    repo = os.environ["SUBSTRUCTURE_ROOT"]
    if not os.path.exists(outputdir):
        os.makedirs(outputdir, 0o755)
    logdir = os.path.join(outputdir, "logs")
    if not os.path.exists(logdir):
        os.makedirs(logdir, 0o755)
    logfile = os.path.join(logdir, f"convert_data_{yeartag}.log")
    runscipt = os.path.join(repo, "omnifold", "run_extract_data.sh")
    runcmd = f"{runscipt} {repo} {inputdir} {outputdir} {yeartag}"
    slurm = f"sbatch -N1 -c1 --partition {partition} -J convert_data_{yeartag} -o {logfile}"
    subprocess.call(f"{slurm} {runcmd}", shell=True)

def main():
    parser = argparse.ArgumentParser("submitExtractData.py")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    parser.add_argument("outputdir", metavar="OUTPUTDIR", type=str, help="Output directory")
    parser.add_argument("-p", "--partition", metavar="PARTITION", type=str, default="long", help="Slurm partition")
    args = parser.parse_args()

    inbase = os.path.abspath(args.inputdir)
    outbase = os.path.abspath(args.outputdir)
    for year in ["2017", "2018"]:
        indir = os.path.join(inbase, year, "merged")
        submit(indir, outbase, year, args.partition)

if __name__ == "__main__":
    main()