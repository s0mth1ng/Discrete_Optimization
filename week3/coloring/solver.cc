#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <unordered_set>
#include <vector>

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

std::vector<size_t> FindColoring(const Graph& g, size_t numberOfColors) {
  if (!numberOfColors) {
    throw std::invalid_argument("Number of colors is positive number.");
  }
  CpModelBuilder cp_model;
  Domain colors(0, numberOfColors - 1);
  size_t numberOfNodes = g.GetNumberOfNodes();
  std::vector<IntVar> nodes;
  for (size_t node = 0; node < numberOfNodes; ++node) {
    nodes.push_back(
        cp_model.NewIntVar(colors).WithName("x" + std::to_string(node)));
  }
  for (size_t node = 0; node < numberOfNodes; ++node) {
    for (auto to : g.GetNeighbors(node)) {
      if (node < to) {
        cp_model.AddAllDifferent({nodes[node], nodes[to]});
      }
    }
  }
  Model model;
  std::vector<size_t> coloring(numberOfNodes);
  model.Add(NewFeasibleSolutionObserver([&](const CpSolverResponse& response) {
    for (size_t node = 0; node < numberOfNodes; ++node) {
      coloring[node] = SolutionIntegerValue(response, nodes[node]);
    }
  }));

  SatParameters parameters;
  parameters.set_enumerate_all_solutions(false);
  model.Add(NewSatParameters(parameters));
  const CpSolverResponse response = SolveCpModel(cp_model.Build(), &model);
  return coloring;
}

}  // namespace sat

std::vector<size_t> FindMaxClique(const Graph& g) {
  /* TODO:
   * Find max clique
   * Use it for break symmetry
   */
  return {};
}
}  // namespace operations_research

void solve(std::istream& in, std::ostream& out) {
  Graph g = InputGraph(in);
  auto coloring = operations_research::sat::FindColoring(g, 2);
  for (auto i : coloring) {
    out << i << ' ';
  }
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    throw std::runtime_error("Usage: ./" + std::string(argv[0]) + " filename");
  }
  std::ifstream fin(argv[1]);
  solve(fin, std::cout);
  return 0;
}