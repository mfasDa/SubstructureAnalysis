#! /usr/bin/env python

import os
import sys

if __name__ == "__main__":
    script=os.path.join(os.path.abspath(os.path.dirname(sys.argv[0])), "extractOutlierTree.cpp")
    basedir = os.getcwd()
    for pthardbin in range(1, 21):
        pthardbindir = os.path.join(os.getcwd(), "%02d" %pthardbin)
        os.chdir(pthardbindir)
        os.system("root -l -b -q %s" %script)
        os.chdir(basedir)