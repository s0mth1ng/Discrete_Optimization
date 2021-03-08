#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <unordered_set>
#include <vector>
using namespace std::chrono;

#include <random>

std::knuth_b rand_engine;
std::uniform_real_distribution<> uniform_zero_to_one(0.0, 1.0);

bool random_bool_with_prob(double prob)  // probability between 0.0 and 1.0
{
  return uniform_zero_to_one(rand_engine) <= prob;
}

#include "ortools/constraint_solver/constraint_solver.h"
#include "ortools/graph/cliques.h"
#include "ortools/graph/graph.h"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/model.h"
#include "ortools/sat/sat_parameters.pb.h"

class Graph {
 public:
  using VertexId = size_t;

  Graph(VertexId nodesCount) : _nodesCount(nodesCount) {
    _g.resize(nodesCount);
  }

  size_t GetNumberOfNodes() const { return _nodesCount; }

  size_t GetNumberOfEdges() const { return _edgesCount; }

  bool IsEdge(VertexId from, VertexId to) const {
    if (std::max(from, to) >= _nodesCount) {
      return false;
    }
    return _g[from].count(to);
  }

  void AddEdge(VertexId from, VertexId to) {
    if (std::max(from, to) >= _nodesCount) {
      throw std::invalid_argument("No such node.");
    }
    auto [it, inserted] = _g[from].insert(to);
    _edgesCount += inserted;
  }

  const std::unordered_set<VertexId>& GetNeighbors(VertexId node) const {
    if (node >= _nodesCount) {
      throw std::invalid_argument("No such node.");
    }
    return _g[node];
  }

 private:
  using NeighborsCont = std::unordered_set<VertexId>;
  std::vector<NeighborsCont> _g;
  VertexId _nodesCount;
  VertexId _edgesCount = 0;
};

struct Solution {
  size_t colorsCount;
  bool isOptimal;
  std::vector<size_t> coloring;
};

std::ostream& operator<<(std::ostream& out, const Solution& solution) {
  out << solution.colorsCount << ' ' << solution.isOptimal << '\n';
  for (auto i : solution.coloring) {
    out << i << ' ';
  }
  return out;
}

struct Duration {
  Duration(size_t ms) {
    h = ms / 1000 / 60 / 60;
    m = ms / 1000 / 60 % 60;
    s = ms / 1000 % 60;
    this->ms = ms % 1000;
  }

  size_t h;
  size_t m;
  size_t s;
  size_t ms;
};

std::ostream& operator<<(std::ostream& out, const Duration& d) {
  if (d.h) {
    out << d.h << ":";
  }
  out << std::setw(2) << std::setfill('0') << d.m << ":" << std::setw(2)
      << std::setfill('0') << d.s << "." << std::setw(3) << std::setfill('0')
      << d.ms;
  return out;
}

Graph InputGraph(std::istream& in) {
  size_t nodesCount, edgesCount;
  in >> nodesCount >> edgesCount;
  Graph g(nodesCount);
  for (size_t i = 0; i < edgesCount; ++i) {
    Graph::VertexId from, to;
    in >> from >> to;
    g.AddEdge(from, to);
    g.AddEdge(to, from);
  }
  return g;
}

namespace operations_research {
namespace sat {

bool FindColoring(const Graph& g,
                  const std::vector<size_t>& clique,
                  size_t maxColors,
                  std::vector<size_t>& coloring,
                  size_t& colorsUsed) {
  size_t numberOfNodes = g.GetNumberOfNodes();
  if (!numberOfNodes) {
    throw std::invalid_argument("Number of colors is positive number.");
  }
  CpModelBuilder cp_model;
  Domain colors(0, maxColors - 1);
  std::vector<IntVar> nodes;
  for (size_t node = 0; node < numberOfNodes; ++node) {
    nodes.push_back(
        cp_model.NewIntVar(colors).WithName("x" + std::to_string(node)));
  }

  // breaking symmetry
  size_t cliqueColor = 0;
  for (auto node : clique) {
    cp_model.AddEquality(nodes[node], cliqueColor);
    cliqueColor++;
  }

  // main constraints of coloring problem
  for (size_t node = 0; node < numberOfNodes; ++node) {
    for (auto to : g.GetNeighbors(node)) {
      if (node < to) {
        cp_model.AddAllDifferent({nodes[node], nodes[to]});
      }
    }
  }

  // Constraint for minimizing number of colors
  IntVar maxColor = cp_model.NewIntVar({(int64) cliqueColor - 1, (int64) maxColors - 1});
  cp_model.AddMaxEquality(maxColor, nodes);
  cp_model.Minimize(maxColor);

  // TODO: constraints for all colors used

  // model
  Model model;
  model.Add(NewFeasibleSolutionObserver([&](const CpSolverResponse& response) {
    colorsUsed = SolutionIntegerValue(response, maxColor) + 1;
    std::cerr << "Coloring in " << colorsUsed << " colors found!\r";
    for (size_t node = 0; node < numberOfNodes; ++node) {
      coloring[node] = SolutionIntegerValue(response, nodes[node]);
    }
  }));

  SatParameters parameters;
  parameters.set_enumerate_all_solutions(false);
  parameters.set_max_time_in_seconds(60 * 5);
  model.Add(NewSatParameters(parameters));
  const CpSolverResponse response = SolveCpModel(cp_model.Build(), &model);
  std::cerr << std::endl;
  return (response.status() == CpSolverStatus::FEASIBLE ||
          response.status() == CpSolverStatus::OPTIMAL);
}

}  // namespace sat

std::vector<size_t> FindMaxClique(const Graph& g) {
  BronKerboschAlgorithm<size_t>::IsArcCallback gCall =
      [&g](size_t node1, size_t node2) { return g.IsEdge(node1, node2); };
  std::vector<size_t> clique;
  BronKerboschAlgorithm<size_t>::CliqueCallback cCall =
      [&clique](const std::vector<size_t>& nodes) {
        clique = nodes;
        return CliqueResponse::STOP;
      };
  BronKerboschAlgorithm<size_t> al(gCall, g.GetNumberOfNodes(), cCall);
  auto res = al.Run();
  return clique;
}
}  // namespace operations_research

std::optional<Solution> ConstraintProgrammingSolution(const Graph& g,
                                                      size_t maxColors,
                                                      size_t maxTimeInSeconds) {
  auto start = high_resolution_clock::now();
  std::cerr << "Starting cp solution...\n";
  size_t numberOfNodes = g.GetNumberOfNodes();
  size_t numberOfEdges = g.GetNumberOfEdges();
  auto clique = operations_research::FindMaxClique(g);
  std::cerr << "Clique size: " << clique.size() << std::endl;
  std::vector<size_t> coloring(numberOfNodes);
  size_t colorsUsed = 0;
  maxColors =
      std::min((size_t)(0.5 + std::sqrt(2 * numberOfEdges + 0.25)), maxColors);
  bool found = operations_research::sat::FindColoring(g, clique, maxColors,
                                                      coloring, colorsUsed);
  auto end = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(end - start);
  std::cerr << std::endl
            << "Time spent: " << Duration(duration.count()) << std::endl;
  if (found) {
    Solution solution;
    solution.colorsCount = colorsUsed;
    solution.isOptimal = false;
    solution.coloring = coloring;
    return solution;
  }
  return std::nullopt;
}

void StartColoring(const Graph& g, size_t start, Solution& solution) {
  size_t numberOfNodes = g.GetNumberOfNodes();
  std::unordered_set<size_t> prohibitedColors;
  for (auto to : g.GetNeighbors(start)) {
    if (solution.coloring[to] != numberOfNodes) {
      prohibitedColors.insert(solution.coloring[to]);
    }
  }
  for (size_t color = 0; color < numberOfNodes; ++color) {
    if (!prohibitedColors.count(color)) {
      solution.coloring[start] = color;
      solution.colorsCount = std::max(solution.colorsCount, color);
      break;
    }
  }
  for (auto to : g.GetNeighbors(start)) {
    if (solution.coloring[to] == numberOfNodes) {
      StartColoring(g, to, solution);
    }
  }
}

Solution GreedySolution(const Graph& g) {
  size_t numberOfNodes = g.GetNumberOfNodes();
  size_t numberOfEdges = g.GetNumberOfEdges();
  size_t nodeWithMostEdges = 0;
  for (size_t node = 1; node < numberOfNodes; ++node) {
    if (g.GetNeighbors(node).size() >
        g.GetNeighbors(nodeWithMostEdges).size()) {
      nodeWithMostEdges = node;
    }
  }
  Solution solution;
  solution.coloring.resize(numberOfNodes, numberOfNodes);
  solution.isOptimal = false;
  solution.colorsCount = 0;
  StartColoring(g, nodeWithMostEdges, solution);
  return solution;
}

using ColorClass = std::unordered_set<size_t>;

size_t ComputeObjectiveFunction(const std::vector<ColorClass>& classes) {
  return std::accumulate(classes.begin(), classes.end(), 0lu,
                         [](size_t sqSum, const ColorClass& cl) {
                           return sqSum + cl.size() * cl.size();
                         });
}

void FindChain(const Graph& g,
               size_t node,
               const std::vector<size_t>& coloring,
               size_t leftColor,
               size_t rightColor,
               bool toRight,
               std::vector<int>& used) {
  used[node] = (toRight ? 1 : 2);
  size_t newColor = (toRight ? rightColor : leftColor);
  for (auto to : g.GetNeighbors(node)) {
    if (!used[to] && coloring[to] == newColor) {
      FindChain(g, to, coloring, leftColor, rightColor, !toRight, used);
    }
  }
}

bool MakeSwapWithProbability(const Graph& g,
                             size_t node,
                             size_t newColor,
                             std::vector<size_t>& coloring,
                             std::vector<ColorClass>& classes,
                             double swappingIfBad = 0.0) {
  size_t prevColor = coloring[node];
  if (classes[prevColor].find(node) == classes[prevColor].end()) {
    throw std::invalid_argument("Node is not colored in this color.");
  }
  std::vector<int> used(g.GetNumberOfNodes(), 0);
  FindChain(g, node, coloring, prevColor, newColor, true, used);
  size_t prevValue = ComputeObjectiveFunction(classes);
  size_t ones = std::count(used.begin(), used.end(), 1);
  size_t twos = std::count(used.begin(), used.end(), 2);
  size_t diff = twos - ones;
  size_t newValue =
      prevValue - classes[prevColor].size() * classes[prevColor].size() -
      classes[newColor].size() * classes[newColor].size() +
      (classes[newColor].size() - diff) * (classes[newColor].size() - diff) +
      (classes[prevColor].size() + diff) * (classes[prevColor].size() + diff);
  bool swapping =
      (newValue >= prevValue ? true : random_bool_with_prob(swappingIfBad));
  if (!swapping) {
    return false;
  }
  for (size_t node = 0; node < g.GetNumberOfNodes(); ++node) {
    if (!used[node]) {
      continue;
    }
    classes[coloring[node]].erase(node);
    size_t curColor = (used[node] == 1 ? newColor : prevColor);
    classes[curColor].insert(node);
    coloring[node] = curColor;
  }
  return true;
}

size_t ComputeNumberOfColorsUsed(const std::vector<size_t>& coloring) {
  return std::set<size_t>(coloring.begin(), coloring.end()).size();
}

void CorrectSolution(Solution& solution) {
  std::unordered_map<size_t, size_t> mapping;
  std::set<size_t> colorsUsed(solution.coloring.begin(),
                              solution.coloring.end());
  size_t cur = 0;
  for (auto c : colorsUsed) {
    mapping[c] = cur++;
  }
  for (auto& c : solution.coloring) {
    c = mapping[c];
  }
}

Solution LocalSearchSolution(const Graph& g) {
  auto start = high_resolution_clock::now();
  std::cerr << "Starting local search solution...\n";
  size_t numberOfNodes = g.GetNumberOfNodes();
  auto initSolution = GreedySolution(g);
  std::vector<ColorClass> classes(numberOfNodes);
  for (size_t node = 0; node < numberOfNodes; ++node) {
    classes[initSolution.coloring[node]].insert(node);
  }
  size_t currentValue = ComputeObjectiveFunction(classes);
  size_t maxMinutes = 120;
  Solution best = initSolution;
  size_t time = 0;
  double initProb = 0.0;
  size_t nIterations = 30;
  for (size_t it = 1; it <= nIterations; ++it) {
    bool swapped = false;
    bool found = false;
    for (size_t node = 0; node < numberOfNodes; ++node) {
      for (size_t color = 0; color < numberOfNodes; ++color) {
        time++;
        if (initSolution.coloring[node] == color) {
          continue;
        }
        size_t prevColor = initSolution.coloring[node];
        swapped = MakeSwapWithProbability(g, node, color, initSolution.coloring,
                                          classes);
        if (swapped) {
          found = true;
          initSolution.colorsCount =
              ComputeNumberOfColorsUsed(initSolution.coloring);
          if (initSolution.colorsCount < best.colorsCount) {
            best = initSolution;
            std::cerr << "Coloring in " << best.colorsCount
                      << " colors found!\r";
          }
        }
      }
    }
    if (it % 10 == 0) {
      std::cerr << '\n' << it << " iterations passed!" << std::endl;
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    if (duration.count() > maxMinutes * 60 * 1000 || !found) {
      std::cerr << "\nTime limit exceeded or swapping did not occur!\n";
      break;
    }
  }
  CorrectSolution(best);
  return best;
}

void solve(std::istream& in, std::ostream& out) {
  Graph g = InputGraph(in);
  if (g.GetNumberOfNodes() == 250 || g.GetNumberOfNodes() == 1000) {
    auto LSSolution = LocalSearchSolution(g);
    out << LSSolution;
    return;
  }
  auto solution = GreedySolution(g);
  size_t time = 5 * 60;
  out << *ConstraintProgrammingSolution(g, solution.colorsCount, time);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    throw std::runtime_error("Usage: ./" + std::string(argv[0]) + " filename");
  }
  std::ifstream fin(argv[1]);
  solve(fin, std::cout);
  return 0;
}