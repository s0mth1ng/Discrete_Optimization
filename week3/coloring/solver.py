#!/usr/bin/python
# -*- coding: utf-8 -*-

from collections import namedtuple
import math
import os
import sys
from subprocess import Popen, PIPE


if __name__ == '__main__':
    import sys
    if len(sys.argv) > 1:
        file_location = sys.argv[1].strip()
        bin_path = '/home/maxim/soft/OrTools/or-tools_Ubuntu-20.04-64bit_v8.1.8487/bin/solver'
        process = Popen([bin_path, file_location], stdout=PIPE, universal_newlines=True)
        (stdout, stderr) = process.communicate()
        print(stdout.strip())
    else:
        print('This test requires an input file.  Please select one from the data directory. (i.e. python solver.py ./data/ks_4_0)')
