#! /usr/bin/env python

import os
import subprocess
import sys

if __name__ == "__main__":
    repo = os.path.dirname(os.path.abspath(sys.argv[0]))
    script = "makeCombinedSystematicUncertainty.cpp"
    for trg in ["INT7", "EJ1", "EJ2"]:
        for r in range(2, 6):
            radius = float(r)/10.
            cmd="root -l -b -q \'%s(%f, \"%s\")\'" %(os.path.join(repo, script), radius, trg)
            subprocess.call(cmd, shell = True)