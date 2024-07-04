#! /usr/bin/env python3

import argparse
import os
import subprocess

def submit(inputdir: str, outputdir: str, partition: str = "long"):
    repo = os.environ["SUBSTRUCTURE_ROOT"]
    pthardbins = len([x for x in os.listdir(inputdir) if x.isdigit()])
    print(f"Found {pthardbins} pt-hard bins")
    if not os.path.exists(outputdir):
        os.makedirs(outputdir, 0o755)
    logdir = os.path.join(outputdir, "logs")
    if not os.path.exists(logdir):
        os.makedirs(logdir, 0o755)
    logfile = os.path.join(logdir, "convert_pthard%a.log")
    runscipt = os.path.join(repo, "omnifold", "run_extract_response.sh")
    runcmd = f"{runscipt} {repo} {inputdir} {outputdir} "
    slurm = f"sbatch -N1 -c1 --partition {partition} -J convert_pthard -o {logfile} --array=1-{pthardbins}"
    subprocess.call(f"{slurm} {runcmd}", shell=True)

if __name__ == "__main__":
    parser = argparse.ArgumentParser("submitExtractResponse.py")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    parser.add_argument("outputdir", metavar="OUTPUTDIR", type=str, help="Output directory")
    parser.add_argument("-p", "--partition", metavar="PARTITION", type=str, default="long", help="Slurm partition")
    args = parser.parse_args()

    inbase = os.path.abspath(args.inputdir)
    outbase = os.path.abspath(args.outputdir)
    for year in ["2017", "2018"]:
        indir = os.path.join(inbase, year, "merged")
        outdir = os.path.join(outbase, year)
        submit(indir, outdir, args.partition)