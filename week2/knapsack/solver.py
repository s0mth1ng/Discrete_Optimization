#!/usr/bin/python
# -*- coding: utf-8 -*-

from collections import namedtuple
import math
import os
from subprocess import Popen, PIPE
Item = namedtuple("Item", ['index', 'value', 'weight'])


def run_cpp_solution(input_data):
    # Writes the inputData to a temporay file
    tmp_file_name = 'tmp.data'
    tmp_file = open(tmp_file_name, 'w')
    tmp_file.write(input_data)
    tmp_file.close()

    process = Popen(['./solver.out', tmp_file_name],
                    stdout=PIPE, universal_newlines=True)
    (stdout, stderr) = process.communicate()

    # removes the temporay file
    os.remove(tmp_file_name)
    return stdout.strip()


def solve_it(input_data):
    return run_cpp_solution(input_data)


if __name__ == '__main__':
    import sys
    if len(sys.argv) > 1:
        file_location = sys.argv[1].strip()
        with open(file_location, 'r') as input_data_file:
            input_data = input_data_file.read()
        print(solve_it(input_data))
    else:
        print('This test requires an input file.  Please select one from the data directory. (i.e. python solver.py ./data/ks_4_0)')
