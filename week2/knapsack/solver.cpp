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

struct Choice {
  uint64_t value;
  bool taken;
};

struct Solution {
  uint64_t value;
  bool isBest;
  std::vector<bool> taken;
};

std::ostream& operator<<(std::ostream& out, const Solution& solution) {
  out << solution.value << ' ' << solution.isBest << '\n';
  for (auto i : solution.taken) {
    out << i << ' ';
  }
  return out;
}

Solution bruteForceSolution(const std::vector<Item>& items, uint64_t capacity) {
  size_t itemCount = items.size(), finalMask = 0;
  uint64_t maxValue = 0;
  for (size_t mask = 0; mask < (1lu << itemCount); ++mask) {
    uint64_t weight = 0, value = 0;
    size_t curMask = mask;
    for (const auto& item : items) {
      if (curMask % 2) {
        weight += item.weight;
        value += item.value;
      }
      curMask >>= 1;
    }
    if (weight <= capacity && value > maxValue) {
      maxValue = value;
      finalMask = mask;
    }
  }
  std::vector<bool> taken(itemCount);
  for (size_t i = 0; i < itemCount; ++i) {
    taken[itemCount - i - 1] = (finalMask & 1);
    finalMask >>= 1;
  }
  return Solution{.value = maxValue, .isBest = true, .taken = taken};
}

Solution dpSolution(const std::vector<Item>& items, uint64_t capacity) {
  size_t itemCount = items.size();
  std::vector<std::vector<Choice>> dp(itemCount,
                                      std::vector<Choice>(capacity + 1));
  for (size_t i = items[0].weight; i <= capacity; ++i) {
    dp[0][i] = {items[0].value, true};
  }

  for (size_t i = 1; i < itemCount; ++i) {
    for (size_t maxWeight = 0; maxWeight <= capacity; ++maxWeight) {
      if (items[i].weight > maxWeight ||
          dp[i - 1][maxWeight].value >=
              dp[i - 1][maxWeight - items[i].weight].value + items[i].value) {
        dp[i][maxWeight] = {dp[i - 1][maxWeight].value, false};
      } else {
        dp[i][maxWeight] = {
            dp[i - 1][maxWeight - items[i].weight].value + items[i].value,
            true};
      }
    }
  }

  std::vector<bool> taken(itemCount);
  int curItem = itemCount - 1;
  size_t curWeight = capacity;
  while (curItem >= 0) {
    if (dp[curItem][curWeight].taken) {
      taken[curItem] = true;
      curWeight -= items[curItem].weight;
    }
    curItem--;
  }

  return Solution{.value = dp[itemCount - 1][capacity].value,
                  .isBest = true,
                  .taken = taken};
}

Solution defaultSolution(std::vector<Item>& items, uint64_t capacity) {
  // greedy algo
  size_t itemCount = items.size();
  std::vector<bool> taken(itemCount, false);
  uint64_t weight = 0, value = 0;
  std::sort(items.begin(), items.end(), [](const auto& lhs, const auto& rhs) {
    return lhs.value * rhs.weight > lhs.weight * rhs.value;
  });
  for (const auto& item : items) {
    if (weight + item.weight <= capacity) {
      weight += item.weight;
      value += item.value;
      taken[item.index] = true;
    } else {
      break;
    }
  }
  return Solution{.value = value, .isBest = false, .taken = taken};
}

struct Node {
  uint64_t value;
  uint64_t room;
  uint64_t estimate;
  std::vector<bool> taken;

  bool operator<(const Node& node) const { return estimate < node.estimate; }
};

Solution smartSearch(std::vector<Item>& items, uint64_t capacity) {
  uint64_t estimated = std::accumulate(
      begin(items), end(items), 0Lu,
      [](uint64_t sum, const auto& i) { return sum + i.value; });
  std::sort(items.begin(), items.end(), [](const auto& lhs, const auto& rhs) {
    return lhs.value * rhs.weight > lhs.weight * rhs.value;
  });
  std::priority_queue<Node> q;
  q.push({0Lu, capacity, estimated, {}});
  uint64_t bestValue = 0;
  std::vector<bool> bestTaken;
  size_t counter = 0;
  while (!q.empty()) {
    counter++;
    auto cur = q.top();
    q.pop();
    if (cur.value > bestValue) {
      bestValue = cur.value;
      bestTaken = cur.taken;
    }
    size_t ind = cur.taken.size();
    if (estimated < bestValue || items.size() == ind || counter >= 4e6) {
      continue;
    }
    auto tmp = cur.taken;
    if (cur.room >= items[ind].weight) {
      tmp.push_back(true);
      q.push({cur.value + items[ind].value, cur.room - items[ind].weight,
              estimated, tmp});
      tmp.pop_back();
    }
    tmp.push_back(false);
    q.push({cur.value, cur.room, estimated - items[ind].value, tmp});
  }
  while (bestTaken.size() != items.size()) {
    bestTaken.push_back(false);
  }
  std::vector<bool> taken(items.size());
  for (size_t i = 0; i < items.size(); ++i) {
    taken[items[i].index] = bestTaken[i];
  }
  return {bestValue, false, taken};
}

Solution solve(std::istream& in) {
  size_t itemCount;
  uint64_t capacity;
  in >> itemCount >> capacity;
  std::vector<Item> items(itemCount);
  size_t ind = 0;
  for (auto& item : items) {
    in >> item.value >> item.weight;
    item.index = ind++;
  }

  if (itemCount < 25) {
    return bruteForceSolution(items, capacity);
  }

  if (itemCount * capacity < 5e7) {
    return dpSolution(items, capacity);
  }
  return smartSearch(items, capacity);
  return defaultSolution(items, capacity);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    throw std::runtime_error("Usage: ./" + std::string(argv[0]) + " filename");
  }
  std::ifstream fin(argv[1]);
  std::cout << solve(fin);
  return 0;
}