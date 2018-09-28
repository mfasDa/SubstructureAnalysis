#! /usr/bin/env python

import os
import subprocess
import sys

if __name__ == "__main__":
    repo = os.path.dirname(os.path.abspath(sys.argv[0]))
    script = "makeCombinedSystematicUncertaintyRatio1D.cpp"
    for r in range(3, 6):
        radius = float(r)/10.
        cmd="root -l -b -q \'%s(0.2, %f)\'" %(os.path.join(repo, script), radius)
        subprocess.call(cmd, shell = True)