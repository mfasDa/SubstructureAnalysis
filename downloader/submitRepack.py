#! /usr/bin/env python3

from argparse import ArgumentParser
from SubstructureHelpers.slurm import submit
import os

if __name__ == "__main__":
    repo = os.path.abspath(os.getenv("SUBSTRUCTURE_ROOT"))
    parser = ArgumentParser("submitRepack.py", description="Submitter for repacking")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    parser.add_argument("-p", "--partition", metavar="PARTITION", type=str, default="short", help="Partition")
    args = parser.parse_args()

    executable = os.path.join(repo, "downloader", "runRepackStandalone.sh")
    cmd = "{EXE} {WD}".format(EXE=executable, WD=args.inputdir)
    logfile = os.path.join(args.inputdir, "repack.log")
    submit(cmd, "repack", logfile, args.partition)