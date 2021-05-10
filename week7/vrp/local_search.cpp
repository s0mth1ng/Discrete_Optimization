#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <unordered_set>
#include <vector>

struct Point {
  double x;
  double y;

  friend std::istream &operator>>(std::istream &in, Point &p) {
    in >> p.x >> p.y;
    return in;
  }
};

double length(const Point &p1, const Point &p2) {
  double dx = p1.x - p2.x;
  double dy = p1.y - p2.y;
  return std::sqrt(dx * dx + dy * dy);
}

struct Warehouse {
  size_t index;
  int demand;
  Point location;
};

class Solver {
public:
  void ParseFrom(std::istream &in) {
    int numberOfWarehouses;
    in >> numberOfWarehouses >> numberOfVehicles >> capacity;
    warehouses.resize(numberOfWarehouses);
    size_t currIndex = 0;
    for (auto &w : warehouses) {
      in >> w.demand >> w.location;
      w.index = currIndex++;
    }
  }

  struct Solution {
    using Route = std::vector<int>;

    double value = 0.0;
    bool isOptimal = false;
    std::vector<Route> routes;

    explicit Solution(int vehicles) { routes.resize(vehicles, Route(1)); }

    friend std::ostream &operator<<(std::ostream &out,
                                    const Solution &solution) {
      out << solution.value << ' ' << solution.isOptimal << '\n';
      for (const auto &r : solution.routes) {
        for (auto w : r) {
          out << w << ' ';
        }
        out << '\n';
      }
      return out;
    }
  };

  void DumpSolution(const Solution &solution) const {
    std::string filename = "./answers/" + std::to_string(warehouses.size());
    std::ifstream fin(filename);
    bool acceptChange = true;
    if (fin) {
      double oldValue;
      fin >> oldValue;
      acceptChange = (!fin || oldValue > solution.value);
      fin.close();
    }
    if (acceptChange) {
      std::ofstream fout(filename, std::ios::out | std::ios::trunc);
      fout << solution;
    }
  }

  Solution FindSolution() { return GreedySolution(); }

private:
  int numberOfVehicles;
  int capacity;
  std::vector<Warehouse> warehouses;

  Solution GreedySolution() {
    Solution solution(numberOfVehicles);

    std::vector<int> capacities(numberOfVehicles, capacity);
    for (const auto &w : warehouses) {
      if (!w.index) {
        continue;
      }
      for (int v = 0; v < numberOfVehicles; ++v) {
        if (capacities[v] >= w.demand) {
          capacities[v] -= w.demand;
          solution.routes[v].push_back(w.index);
          break;
        }
      }
    }
    for (auto &r : solution.routes) {
      r.push_back(0);
      for (size_t i = 1; i < r.size(); ++i) {
        solution.value +=
            length(warehouses[i - 1].location, warehouses[i].location);
      }
    }

    return solution;
  }
};

void solve(std::istream &in) {
  Solver solver;
  solver.ParseFrom(in);
  auto solution = solver.FindSolution();
  solver.DumpSolution(solution);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>\n";
    return EXIT_FAILURE;
  }
  std::ifstream fin(argv[1]);
  if (!fin) {
    std::cerr << "File not found.\n";
    return EXIT_FAILURE;
  }
  solve(fin);
}