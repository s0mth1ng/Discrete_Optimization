from __future__ import print_function
import sys
from ortools.linear_solver import pywraplp
from collections import namedtuple
import math
import numpy as np
from timeit import default_timer as timer


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


Point = namedtuple("Point", ['x', 'y'])
Facility = namedtuple(
    "Facility", ['index', 'setup_cost', 'capacity', 'location'])
Customer = namedtuple("Customer", ['index', 'demand', 'location'])


def length(point1, point2):
    return math.sqrt((point1.x - point2.x) ** 2 + (point1.y - point2.y) ** 2)


class Solution:
    def __init__(self):
        self.attendance: list[int] = []
        self.value: float = 0.0
        self.optimal: bool = False

    def __repr__(self):
        result = f'{self.value} {int(self.optimal)}\n'
        result += ' '.join(map(str, self.attendance))
        return result


class Solver:
    def __init__(self, facilities: list[Facility], customers: list[Customer]):
        self.facilities = facilities
        self.customers = customers
        self.n_facilities = len(facilities)
        self.n_customers = len(customers)
        self.__init_distances()

    def __init_distances(self):
        self.__DISTANCE_TO_CUSTOMERS = np.zeros(
            (self.n_facilities, self.n_customers))
        self.__DISTANCE_TO_FACILITIES = np.zeros(
            (self.n_facilities, self.n_facilities))

        # Compute distances from facilities to customers
        for f in range(self.n_facilities):
            for c in range(self.n_customers):
                self.__DISTANCE_TO_CUSTOMERS[f][c] = length(
                    self.facilities[f].location, self.customers[c].location)
            for other_f in range(self.n_facilities):
                self.__DISTANCE_TO_FACILITIES[f][other_f] = length(
                    self.facilities[f].location, self.facilities[other_f].location)

        # Create nearest neighbors matrix
        self.__NEAREST_NEIGHBORS = [
            list(range(self.n_facilities)) for _ in range(self.n_facilities)]
        for f in range(self.n_facilities):
            self.__NEAREST_NEIGHBORS[f].sort(
                key=lambda other_f: self.__DISTANCE_TO_FACILITIES[f][other_f])

    def __get_initial_solution(self) -> Solution:
        solution = Solution()
        capacities = [f.capacity for f in self.facilities]
        opened = [False] * self.n_facilities
        for i, c in enumerate(self.customers):
            for j, f in enumerate(self.facilities):
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

    def __find_optimal_solution(self, f, c, max_seconds=60):
        n_facilities = len(f)
        n_customers = len(c)

        # Init MIP solver
        solver = pywraplp.Solver('SolveIntegerProblem',
                                 pywraplp.Solver.CBC_MIXED_INTEGER_PROGRAMMING)

        # Init variables
        attendance = [[solver.IntVar(0, 1, f'y_{i}_{j}') for j in range(
            n_customers)] for i in range(n_facilities)]
        opened = [solver.IntVar(0, 1, f'opened_{i}')
                  for i in range(n_facilities)]

        # Add condition: all customers must be served by exactly 1 facility
        for j in range(n_customers):
            solver.Add(sum([attendance[i][j]
                       for i in range(n_facilities)]) == 1)

        # Add condition: a customer must be assigned to an open facility.
        for i in range(n_facilities):
            for j in range(n_customers):
                solver.Add(attendance[i][j] <= opened[i])

        # Add condition: sum of demands <= capacity
        for i in range(n_facilities):
            solver.Add(sum([self.customers[c[j]].demand * attendance[i][j] for j in range(n_customers)]) <=
                       self.facilities[f[i]].capacity * opened[i])

        # Minimizing function
        objective = solver.Objective()

        # Objective: sum all the distance.
        for i in range(n_facilities):
            for j in range(n_customers):
                objective.SetCoefficient(attendance[i][j],
                                         length(self.facilities[f[i]].location, self.customers[c[j]].location))

        # Objective: sum all the setup cost.
        for j in range(n_facilities):
            objective.SetCoefficient(
                opened[j], self.facilities[f[j]].setup_cost)

        objective.SetMinimization()

        solver.SetTimeLimit(int(max_seconds * 1000))
        status = solver.Solve()
        if status not in [pywraplp.Solver.FEASIBLE, pywraplp.Solver.OPTIMAL]:
            return None

        solution = Solution()
        solution.value = solver.Objective().Value()
        solution.attendance = [0] * self.n_customers
        for i in range(n_customers):
            for j in range(n_facilities):
                if int(attendance[j][i].solution_value()) == 1:
                    solution.attendance[c[i]] = f[j]
                    break
        return solution

    def solve(self, max_seconds: int = 60 * 60 * 2) -> Solution():
        start_time = timer()

        solution = self.__get_initial_solution()
        eprint(f'Initial value: {solution.value}')

        while True:
            # Pick one facility and choose k nearest to it
            pivot = np.random.choice(self.n_facilities)
            K_NEAREST = 50
            picked_facilities = self.__NEAREST_NEIGHBORS[pivot][:K_NEAREST]
            set_picked = set(picked_facilities)

            # Pick customers that are served by picked facilities
            picked_customers = [c for c in range(
                self.n_customers) if solution.attendance[c] in set_picked]

            # Compute old value
            old_value = 0
            opened = set(solution.attendance)
            for f in picked_facilities:
                if f in opened:
                    old_value += self.facilities[f].setup_cost
            for c in picked_customers:
                old_value += self.__DISTANCE_TO_CUSTOMERS[solution.attendance[c]][c]

            # Run MIP solver
            update = self.__find_optimal_solution(picked_facilities, picked_customers)

            if update is None or update.value > old_value:
                continue

            for c in picked_customers:
                solution.attendance[c] = update.attendance[c]
            solution.value -= old_value - update.value
            eprint(f'Found new value: {solution.value:.02f}', end='\r')
            
            end_time = timer()
            if end_time - start_time > max_seconds:
                eprint('Time limit exceeded.')
                break

        return solution


def parse_data(input_data):
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
    return facilities, customers


def solve_it(input_data) -> str:
    solver = Solver(*parse_data(input_data))
    return str(solver.solve())


if __name__ == '__main__':
    if len(sys.argv) == 2:
        file_location = sys.argv[1].strip()
        with open(file_location, 'r') as input_data_file:
            input_data = input_data_file.read()
        answer_file = './answers/' + \
            '_'.join(input_data.split('\n')[0].split())
        with open(answer_file, 'w') as f:
            f.write(solve_it(input_data))
    else:
        print('This test requires an input file. Please select one from the data directory. '
              '(i.e. python solver.py ./data/fl_16_2)')
