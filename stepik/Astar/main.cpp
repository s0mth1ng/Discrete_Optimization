#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <vector>

class Puzzle15Solver {
 public:
  const static int TABLE_WIDTH = 4;
  const static int TABLE_HEIGHT = 4;
  const static int N = TABLE_HEIGHT * TABLE_WIDTH;
  using Table = std::array<std::array<int, TABLE_WIDTH>, TABLE_HEIGHT>;

  Table end{};

  Puzzle15Solver() {
    for (uint8_t i = 0; i + 1 < N; ++i) {
      end[i / TABLE_WIDTH][i % TABLE_WIDTH] = i + 1;
    }
  }

  int64_t GetNumberOfSteps(const Table &start) const {
    std::priority_queue<std::pair<int64_t, Table>> queue;
    queue.push({-ComputePotential(start), start});
    std::map<Table, int64_t> distance;
    distance[start] = 0;
    while (!queue.empty()) {
      auto table = queue.top().second;
      queue.pop();
      if (table == end) {
        break;
      }
      int64_t newDistance = distance[table] + 1;
      for (auto to : GetNeighbors(table)) {
        if (distance.count(to) == 0 || distance[to] > newDistance) {
          distance[to] = newDistance;
          int64_t priority = newDistance + ComputePotential(to);
          queue.push({-priority, to});
        }
      }
    }
    return distance[end];
  }

 private:
  static std::vector<Table> GetNeighbors(const Table &table) {
    int h0 = 0;
    int w0 = 0;
    for (int i = 0; i < TABLE_HEIGHT; ++i) {
      for (int j = 0; j < TABLE_WIDTH; ++j) {
        if (table[i][j] == 0) {
          h0 = i;
          w0 = j;
          break;
        }
      }
    }

    Table next = table;
    std::vector<Table> neighbors;
    for (int dh = -1; dh < 2; ++dh) {
      for (int dw = -1; dw < 2; ++dw) {
        if (std::abs(dh) != std::abs(dw) && IsPositionValid(h0 + dh, w0 + dw)) {
          std::swap(next[h0][w0], next[h0 + dh][w0 + dw]);
          neighbors.push_back(next);
          std::swap(next[h0][w0], next[h0 + dh][w0 + dw]);
        }
      }
    }
    return neighbors;
  }

  static int64_t ComputePotential(const Table &table) {
    int64_t potential = 0;
    for (int i = 0; i < TABLE_HEIGHT; ++i) {
      for (int j = 0; j < TABLE_WIDTH; ++j) {
        std::pair<int, int> truePos = GetPositionByItem(table[i][j]);
        potential += std::abs(i - truePos.first) + std::abs(j - truePos.second);
      }
    }
    return potential;
  }

  static bool IsPositionValid(int h, int w) {
    return (std::min(h, w) >= 0 && h < TABLE_HEIGHT && w < TABLE_WIDTH);
  }

  static std::pair<int, int> GetPositionByItem(int item) {
    item = (item + N - 1) % N;
    return {item / TABLE_WIDTH, item % TABLE_WIDTH};
  }
};

void solve(std::istream &in, std::ostream &out) {
  Puzzle15Solver::Table table;
  for (int i = 0; i < Puzzle15Solver::TABLE_HEIGHT; ++i) {
    for (int j = 0; j < Puzzle15Solver::TABLE_WIDTH; ++j) {
      in >> table[i][j];
    }
  }
  out << Puzzle15Solver().GetNumberOfSteps(table) << '\n';
}

int main() {
#ifdef LOCAL
  std::ifstream fin("input.txt");
  solve(fin, std::cout);
#else
  solve(std::cin, std::cout);
#endif
  return EXIT_SUCCESS;
}