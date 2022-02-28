#! /usr/bin/env python
import sys
from SubstructureHelpers.aodsets import get_current_AODset

if __name__ == "__main__":
    print(get_current_AODset(sys.argv[1]))