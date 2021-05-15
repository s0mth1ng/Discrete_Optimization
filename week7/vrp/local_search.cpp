#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "TSPSolver.h"

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

  double UpdateRouteViaTSP(Solution::Route &route) {
    // Run TSP solver to improve route
    std::vector<Vector> pts(route.size() - 1);
    for (size_t i = 0; i + 1 < route.size(); ++i) {
      pts[i] = Vector(warehouses[route[i]].location.x,
                      warehouses[route[i]].location.y);
    }
    auto tspSolution = LocalSearchSolver(pts).FindSolution(5);
    auto depoIt =
        std::find(tspSolution.indices.begin(), tspSolution.indices.end(), 0);

    // Update route
    auto rCpy = route;
    route.clear();
    for (auto it = depoIt; it != tspSolution.indices.end(); ++it) {
      route.push_back(rCpy[*it]);
    }
    for (auto it = tspSolution.indices.begin(); it != depoIt; ++it) {
      route.push_back(rCpy[*it]);
    }
    route.push_back(0);
    return tspSolution.distance;
  }

  Solution FindSolution() {
    auto solution = GreedySolution();

    std::cerr << "Initial value: " << solution.value << '\n';

    // Local Search
    size_t counter = 0;
    for (auto &r : solution.routes) {
      counter++;
      std::cerr << std::setprecision(2) << std::fixed
                << (counter * 100.0 / numberOfVehicles) << "%\r";
      if (r.size() <= 2) {
        continue;
      }
      // Compute old tour distance
      double oldDistance = ComputeTourDistance(r);

      // Update value
      solution.value -= oldDistance - UpdateRouteViaTSP(r);
    }
    std::cerr << '\n';

    // Random stuff
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<size_t> rdVehicle(0, numberOfVehicles - 1);
    std::uniform_int_distribution<> actionType(0, 1);

    size_t nIters = 100'000;
    for (size_t it = 0; it < nIters; ++it) {
      size_t sourceVehicle = rdVehicle(mt);
      if (solution.routes[sourceVehicle].size() < 3) {
        continue;
      }
      size_t destinationVehicle = rdVehicle(mt);
      if (actionType(mt)) { // trying make transfer
        std::uniform_int_distribution<size_t> rdCustomer(
            1, solution.routes[sourceVehicle].size() - 2);
        size_t targetInd = rdCustomer(mt);
        int whouse = solution.routes[sourceVehicle][targetInd];
        int currentDemand = 0;
        for (auto w : solution.routes[destinationVehicle]) {
          currentDemand += warehouses[w].demand;
        }
        if (warehouses[whouse].demand + currentDemand <= capacity) {
          auto copySolution = solution;
          copySolution.routes[sourceVehicle].erase(
              copySolution.routes[sourceVehicle].begin() + targetInd);
          copySolution.routes[destinationVehicle].pop_back();
          copySolution.routes[destinationVehicle].push_back(whouse);
          copySolution.routes[destinationVehicle].push_back(0);
          double oldDistance =
              ComputeTourDistance(solution.routes[destinationVehicle]) +
              ComputeTourDistance(solution.routes[sourceVehicle]);
          double newDistance =
              UpdateRouteViaTSP(copySolution.routes[destinationVehicle]) +
              ComputeTourDistance(copySolution.routes[sourceVehicle]);
          if (newDistance < oldDistance) {
            copySolution.value = solution.value - oldDistance + newDistance;
            solution = copySolution;
            std::cerr << "New value found: " << solution.value << '\r';
          }
        }
      } else if (solution.routes[destinationVehicle].size() > 2) { // swap
        std::uniform_int_distribution<size_t> rdCustomerFromSource(
            1, solution.routes[sourceVehicle].size() - 2);
        std::uniform_int_distribution<size_t> rdCustomerFromDest(
            1, solution.routes[destinationVehicle].size() - 2);
        auto w1 = rdCustomerFromSource(mt);
        auto w2 = rdCustomerFromDest(mt);
        int currentDemand1 = 0, currentDemand2 = 0;
        for (auto w : solution.routes[sourceVehicle]) {
          currentDemand1 += warehouses[w].demand;
        }
        for (auto w : solution.routes[destinationVehicle]) {
          currentDemand2 += warehouses[w].demand;
        }
        if (currentDemand1 + warehouses[w2].demand > capacity &&
            currentDemand2 + warehouses[w1].demand > capacity) {
          continue;
        }
        auto copySolution = solution;
        std::swap(copySolution.routes[sourceVehicle][w1],
                  copySolution.routes[destinationVehicle][w2]);
        double oldDistance =
            ComputeTourDistance(solution.routes[destinationVehicle]) +
            ComputeTourDistance(solution.routes[sourceVehicle]);
        double newDistance =
            UpdateRouteViaTSP(copySolution.routes[destinationVehicle]) +
            UpdateRouteViaTSP(copySolution.routes[sourceVehicle]);
        if (newDistance < oldDistance) {
          copySolution.value = solution.value - oldDistance + newDistance;
          solution = copySolution;
          std::cerr << "New value found: " << solution.value << '\r';
        }
      }
    }
    return solution;
  }

private:
  int numberOfVehicles;
  int capacity;
  std::vector<Warehouse> warehouses;

  double ComputeTourDistance(const Solution::Route &route) const {
    double distance = 0;
    for (size_t i = 1; i < route.size(); ++i) {
      distance += length(warehouses[route[i - 1]].location,
                         warehouses[route[i]].location);
    }
    return distance;
  }

  Solution GreedySolution() {
    Solution solution(numberOfVehicles);
    auto warehousesCopy = warehouses;
    sort(warehousesCopy.begin(), warehousesCopy.end(),
         [](const Warehouse &w1, const Warehouse &w2) {
           return w1.demand > w2.demand;
         });
    std::vector<int> capacities(numberOfVehicles, capacity);
    for (const auto &w : warehousesCopy) {
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
      solution.value += ComputeTourDistance(r);
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
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " test1 test2 ...\n";
    return EXIT_FAILURE;
  }

  std::vector<std::string> files = {
      "./data/vrp_16_3_1",   "./data/vrp_26_8_1",   "./data/vrp_51_5_1",
      "./data/vrp_101_10_1", "./data/vrp_200_16_1", "./data/vrp_421_41_1"};

  for (int i = 1; i < argc; ++i) {
    int test = atoi(argv[i]);
    std::cerr << "Running test " << test << '\n';
    std::ifstream fin(files[test - 1]);
    solve(fin);
  }
}