#! /usr/bin/env python

import os
import subprocess
import sys

if __name__ == "__main__":
    repo = os.path.dirname(os.path.abspath(sys.argv[0]))
    script = "makeCombinedSystematicUncertainty1D.cpp"
    for r in range(2, 6):
        radius = float(r)/10.
        cmd="root -l -b -q \'%s(%f)\'" %(os.path.join(repo, script), radius)
        subprocess.call(cmd, shell = True)