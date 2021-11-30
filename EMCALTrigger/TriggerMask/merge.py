#! /usr/bin/env python3

import argparse
import os
import subprocess
import sys

def create_outfilename(filename: str) -> str:
    filebase = os.path.basename(filename)
    outfilename = filebase[:filebase.find(".")]
    outfilename += "_merged.root"
    return outfilename

def find_files(inputdir: str, filename: str) -> list:
    result = []
    for root, dirs, files in os.walk(inputdir):
        for f in files:
            if not filename in f:
                continue
            result.append(os.path.abspath(os.path.join(root, f)))
    if not len(result):
        raise RuntimeError("No files found in input directory")
    return result

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("filename", metavar="FILENAME", type=str, help="Name of the roofile")
    parser.add_argument("inputdir", metavar="INPUTDIR", type=str, help="Input directory")
    args = parser.parse_args()

    outputfile = os.path.join(os.path.abspath(args.inputdir), create_outfilename(args.filename))
    cmd="hadd -f {}".format(outputfile)
    try:
        for fl in find_files(args.inputdir, args.filename):
            cmd += " {}".format(os.path.abspath(fl))
        subprocess.call(cmd, shell=True)
        sys.exit(0)
    except RuntimeError as e:
        print(e)
        sys.exit(1)
