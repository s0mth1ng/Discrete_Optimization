#!/usr/bin/python
# -*- coding: utf-8 -*-

from collections import namedtuple
Item = namedtuple("Item", ['index', 'value', 'weight'])


def brute_force_solution(items, capacity):
    max_value = 0
    taken = 0
    item_count = len(items)

    for mask in range(2 ** item_count):
        weight = 0
        value = 0
        current_mask = mask
        for i in reversed(items):
            if current_mask % 2 == 1:
                weight += i.weight
                value += i.value
            current_mask //= 2
        if weight <= capacity and value > max_value:
            max_value = value
            taken = mask

    output_data = str(max_value) + ' 1\n' + \
        ' '.join(list(format(taken, f'#0{item_count + 2}b')[2:]))
    return output_data


def dp_solution(items, capacity):
    item_count = len(items)
    dp = [[(0, 0) for _ in range(capacity + 1)] for i in range(item_count)]

    # start values
    for i in range(items[0].weight, capacity + 1):
        dp[0][i] = (items[0].value, 1)

    # reccurence
    for i in range(1, item_count):
        for max_weight in range(capacity + 1):
            if items[i].weight > max_weight:
                dp[i][max_weight] = (dp[i - 1][max_weight][0], 0)
            elif dp[i - 1][max_weight][0] < dp[i - 1][max_weight - items[i].weight][0] + items[i].value:
                dp[i][max_weight] = (
                    dp[i - 1][max_weight - items[i].weight][0] + items[i].value, 1)
            else:
                dp[i][max_weight] = (dp[i - 1][max_weight][0], 0)

    taken = [0] * item_count
    cur_item = item_count - 1
    cur_weight = capacity
    while cur_item >= 0:
        if dp[cur_item][cur_weight][1]:
            taken[cur_item] = 1
            cur_weight -= items[cur_item].weight
        cur_item -= 1

    output_data = str(dp[item_count - 1][capacity][0]) + ' 1\n'
    output_data += ' '.join(map(str, taken))
    return output_data


def solve_it(input_data):
    # Modify this code to run your optimization algorithm

    # parse the input
    lines = input_data.split('\n')

    firstLine = lines[0].split()
    item_count = int(firstLine[0])
    capacity = int(firstLine[1])

    items = []

    for i in range(1, item_count+1):
        line = lines[i]
        parts = line.split()
        items.append(Item(i-1, int(parts[0]), int(parts[1])))

    if item_count <= 22:
        return brute_force_solution(items, capacity)

    if item_count * capacity <= 3 * 10 ** 7:
        return dp_solution(items, capacity)

    # a trivial algorithm for filling the knapsack
    # it takes items in-order until the knapsack is full
    value = 0
    weight = 0
    taken = [0]*len(items)

    for item in items:
        if weight + item.weight <= capacity:
            taken[item.index] = 1
            value += item.value
            weight += item.weight

    # prepare the solution in the specified output format
    output_data = str(value) + ' ' + str(0) + '\n'
    output_data += ' '.join(map(str, taken))
    return output_data


if __name__ == '__main__':
    import sys
    if len(sys.argv) > 1:
        file_location = sys.argv[1].strip()
        with open(file_location, 'r') as input_data_file:
            input_data = input_data_file.read()
        print(solve_it(input_data))
    else:
        print('This test requires an input file.  Please select one from the data directory. (i.e. python solver.py ./data/ks_4_0)')
