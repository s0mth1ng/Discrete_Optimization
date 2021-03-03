#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <queue>
#include <string>
#include <vector>

struct Item {
  uint64_t weight;
  uint64_t value;
  size_t index;
};

std::pair<uint64_t, uint64_t> ComputeBounds(const std::vector<Item>& items,
                                            uint64_t capacity,
                                            size_t startFrom = 0) {
  // greedy algo
  // items already sorted
  uint64_t value = 0, weight = 0, right = 0;
  for (size_t i = startFrom; i < items.size(); ++i) {
    if (items[i].weight + weight > capacity && !right) {
      right = value + (uint64_t)(items[i].value * 1.0 / items[i].weight *
                                 (capacity - weight));
    }
    if (items[i].weight + weight <= capacity) {
      value += items[i].value;
      weight += items[i].weight;
    }
  }
  return {value, right};
}

void Dfs(const std::vector<Item>& items,
         uint64_t capacity,
         size_t processed,
         uint64_t currentValue,
         uint64_t& bestValue) {
  bestValue = std::max(bestValue, currentValue);
  if (processed == items.size()) {
    return;
  }
  auto bounds = ComputeBounds(items, capacity, processed);
  bestValue = std::max(bestValue, currentValue + bounds.first);
  if (bounds.second + currentValue <= bestValue) {
    return;
  }
  if (items[processed].weight <= capacity) {
    Dfs(items, capacity - items[processed].weight, processed + 1,
        currentValue + items[processed].value, bestValue);
  }
  Dfs(items, capacity, processed + 1, currentValue, bestValue);
}

uint64_t BBSolution(std::vector<Item>& items, uint64_t capacity) {
  std::sort(items.begin(), items.end(), [](const auto& lhs, const auto& rhs) {
    return lhs.value * rhs.weight > lhs.weight * rhs.value;
  });
  auto bounds = ComputeBounds(items, capacity);
  auto bestValue = bounds.first;
  Dfs(items, capacity, 0, 0, bestValue);
  return bestValue;
}

void solve(std::istream& in, std::ostream& out) {
  uint64_t capacity;
  size_t itemCount;
  in >> capacity >> itemCount;
  std::vector<Item> items(itemCount);
  for (size_t i = 0; i < itemCount; ++i) {
    in >> items[i].weight >> items[i].value;
    items[i].index = i;
  }
  out << BBSolution(items, capacity) << '\n';
}

int main() {
  solve(std::cin, std::cout);
  return 0;
}