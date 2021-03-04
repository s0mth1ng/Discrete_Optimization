#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <unordered_set>
#include <vector>

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
  IntVar maxColor = cp_model.NewIntVar({cliqueColor - 1, maxColors - 1});
  cp_model.AddMaxEquality(maxColor, nodes);
  cp_model.Minimize(maxColor);

  // TODO: constraints for all colors used

  // model
  Model model;
  model.Add(NewFeasibleSolutionObserver([&](const CpSolverResponse& response) {
    colorsUsed = SolutionIntegerValue(response, maxColor) + 1;
    std::cerr << "Coloring in " << colorsUsed << " colors found!" << std::endl;
    for (size_t node = 0; node < numberOfNodes; ++node) {
      coloring[node] = SolutionIntegerValue(response, nodes[node]);
    }
  }));

  SatParameters parameters;
  parameters.set_enumerate_all_solutions(false);
  parameters.set_max_time_in_seconds(60 * 5);
  model.Add(NewSatParameters(parameters));
  const CpSolverResponse response = SolveCpModel(cp_model.Build(), &model);
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

std::optional<Solution> ConstraintProgrammingSolution(const Graph& g, size_t maxColors, size_t maxTimeInSeconds) {
  size_t numberOfNodes = g.GetNumberOfNodes();
  size_t numberOfEdges = g.GetNumberOfEdges();
  auto clique = operations_research::FindMaxClique(g);
  std::cerr << "Clique size: " << clique.size() << std::endl;
  std::vector<size_t> coloring(numberOfNodes);
  size_t colorsUsed = 0;
  maxColors = std::min((size_t)(0.5 + std::sqrt(2 * numberOfEdges + 0.25)), maxColors);
  bool found = operations_research::sat::FindColoring(g, clique, maxColors,
                                                      coloring, colorsUsed);
  if (found) {
    Solution solution;
    solution.colorsCount = colorsUsed;
    solution.isOptimal = false;
    solution.coloring = coloring;
    return solution;
  }
  return std::nullopt;
}

void StartColoring(const Graph &g, size_t start, Solution &solution) {
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

Solution BruteForceSolution(const Graph &g) {
  size_t numberOfNodes = g.GetNumberOfNodes();
  size_t numberOfEdges = g.GetNumberOfEdges();
  size_t nodeWithMostEdges = 0;
  for (size_t node = 1; node < numberOfNodes; ++node) {
    if (g.GetNeighbors(node).size() > g.GetNeighbors(nodeWithMostEdges).size()) {
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

void solve(std::istream& in, std::ostream& out) {
  Graph g = InputGraph(in);
  Solution solution = BruteForceSolution(g);
  size_t time = 60 * 5; // 5 minutes to all subtasks except 4th and 6th
  if (g.GetNumberOfNodes() == 250 || g.GetNumberOfNodes() == 1000) {
    time = 60 * 60; // 1 hour, cause why not?
  }
  auto cpSolution = ConstraintProgrammingSolution(g, solution.colorsCount, time);
  if (cpSolution && cpSolution->colorsCount < solution.colorsCount) {
    solution = *cpSolution;
  }
  out << solution;
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    throw std::runtime_error("Usage: ./" + std::string(argv[0]) + " filename");
  }
  std::ifstream fin(argv[1]);
  solve(fin, std::cout);
  return 0;
}