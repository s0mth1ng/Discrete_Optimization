#!/usr/bin/python
# -*- coding: utf-8 -*-

from __future__ import print_function
import sys
from ortools.linear_solver import pywraplp
from collections import namedtuple
import math


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


Point = namedtuple("Point", ['x', 'y'])
Facility = namedtuple(
    "Facility", ['index', 'setup_cost', 'capacity', 'location'])
Customer = namedtuple("Customer", ['index', 'demand', 'location'])


class Solution:
    def __init__(self):
        self.attendance: list[int] = []
        self.value: float = 0.0
        self.optimal: bool = False

    def __repr__(self):
        result = f'{self.value} {int(self.optimal)}\n'
        result += ' '.join(map(str, self.attendance))
        return result


def length(point1, point2):
    return math.sqrt((point1.x - point2.x) ** 2 + (point1.y - point2.y) ** 2)


def get_initial_solution(facilities: list[Facility], customers: list[Customer]) -> Solution:
    solution = Solution()
    capacities = [f.capacity for f in facilities]
    opened = [False] * len(facilities)
    for i, c in enumerate(customers):
        for j, f in enumerate(facilities):
            if capacities[j] >= c.demand:
                capacities[j] -= c.demand
                solution.attendance.append(f.index)
                solution.value += length(c.location, f.location)
                if not opened[j]:
                    opened[j] = True
                    solution.value += f.setup_cost
                break
        else:
            raise RuntimeError("Data is incorrect")
    return solution


def solve(facilities, customers):
    max_minutes = 30

    n_facilities = len(facilities)
    n_customers = len(customers)

    # Init MIP solver
    solver = pywraplp.Solver.CreateSolver('SCIP')

    # Init variables
    opened = [solver.IntVar(0, 1, f'opened_{i}') for i in range(n_facilities)]
    attendance = [[solver.IntVar(0, 1, f'y_{i}_{j}') for j in range(
        n_customers)] for i in range(n_facilities)]

    # Add condition: all customers must be served by exactly 1 facility
    for j in range(n_customers):
        solver.Add(sum([attendance[i][j] for i in range(n_facilities)]) == 1)

    # Add condition: sum of demands <= capacity
    for i in range(n_facilities):
        solver.Add(sum([customers[j].demand * attendance[i][j] for j in range(n_customers)])
                   <= facilities[i].capacity * opened[i])

    # Minimizing function
    solver.Minimize(sum([opened[i] * facilities[i].setup_cost + sum([attendance[i][j] * length(
        facilities[i].location, customers[j].location) for j in range(n_customers)]) for i in range(n_facilities)]))

    solver.SetTimeLimit(max_minutes * 60 * 1000)
    status = solver.Solve()

    if status == pywraplp.Solver.NOT_SOLVED:
        eprint('Solution not found!')
        return None

    solution = Solution()
    solution.optimal = (status == pywraplp.Solver.OPTIMAL)
    solution.value = solver.Objective().Value()
    eprint(f'Final value: {solution.value:.02f}')
    solution.attendance = [0] * n_customers
    for i in range(n_customers):
        for j in range(n_facilities):
            if int(attendance[j][i].solution_value()) == 1:
                solution.attendance[i] = j
                break
    return str(solution)


def solve_it(input_data):
    # parse the input
    lines = input_data.split('\n')

    parts = lines[0].split()
    facility_count = int(parts[0])
    customer_count = int(parts[1])

    facilities = []
    for i in range(1, facility_count + 1):
        parts = lines[i].split()
        facilities.append(Facility(
            i - 1, float(parts[0]), int(parts[1]), Point(float(parts[2]), float(parts[3]))))

    customers = []
    for i in range(facility_count + 1, facility_count + 1 + customer_count):
        parts = lines[i].split()
        customers.append(Customer(
            i - 1 - facility_count, int(parts[0]), Point(float(parts[1]), float(parts[2]))))

    return solve(facilities, customers)


if __name__ == '__main__':
    if len(sys.argv) > 1:
        file_location = sys.argv[1].strip()
        with open(file_location, 'r') as input_data_file:
            input_data = input_data_file.read()
        print(solve_it(input_data))
    else:
        print('This test requires an input file. Please select one from the data directory. '
              '(i.e. python solver.py ./data/fl_16_2)')
