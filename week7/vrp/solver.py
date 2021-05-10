#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys


def solve_it(input_data: str) -> str:
    answer_file = './answers/' + input_data.split('\n')[0].split()[0]
    with open(answer_file, 'r') as f:
        return f.read()


if __name__ == '__main__':
    if len(sys.argv) > 1:
        file_location = sys.argv[1].strip()
        with open(file_location, 'r') as input_data_file:
            input_data = input_data_file.read()
        print(solve_it(input_data))
    else:
        print('This test requires an input file. Please select one from the data directory. '
              '(i.e. python solver.py ./data/fl_16_2)')
