from collections import namedtuple
import numpy as np
from scipy.optimize import linprog

Row = namedtuple('Row', 'index weight covered_cols')


def compute_value(rows, n_cols, cover):
    covered = set()
    value = 0
    for r in rows:
        if not cover[r.index]:
            continue
        for c in r.covered_cols:
            covered.add(c)
        value += r.weight
    return (len(covered) == n_cols, value)


def lp_solution(rows, n_cols):
    n_rows = len(rows)
    weights = np.array([r.weight for r in rows])
    incidence = np.zeros((n_rows, n_cols), 'int')
    for r in rows:
        for c in r.covered_cols:
            incidence[r.index][c] = 1
    solution = linprog(weights, A_ub=-incidence.T, b_ub=-
                       np.ones((n_cols, 1)), bounds=(0, None))
    n_iters = 100_000
    best_cover = None
    best_value = None
    for _ in range(n_iters):
        cover = np.random.rand(n_rows) > solution.x
        is_cover, value = compute_value(rows, n_cols, cover)
        if is_cover and (best_value is None or best_value > value):
            best_value = value
            best_cover = cover
    if best_value is None:
        print('Solution not found!')
    else:
        answer = np.arange(1, n_rows + 1)[best_cover]
        print(' '.join(map(str, answer)))


def solve():
    n_cols, n_rows = map(int, input().split())
    rows = []
    for i in range(n_rows):
        weight, *covered_cols = input().split()
        rows.append(Row(i, int(weight), np.array(covered_cols, 'int')))
    lp_solution(rows, n_cols)


if __name__ == '__main__':
    solve()
