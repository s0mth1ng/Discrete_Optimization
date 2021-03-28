#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <queue>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Point2D {
public:
  using CoordType = long double;

  Point2D(CoordType x, CoordType y) : _x(x), _y(y) {}
  Point2D() : _x(0), _y(0) {}

  CoordType Distance(const Point2D &other) const {
    return std::sqrt((other._x - _x) * (other._x - _x) +
                     (other._y - _y) * (other._y - _y));
  }

  friend std::istream &operator>>(std::istream &in, Point2D &pt) {
    in >> pt._x >> pt._y;
    return in;
  }

private:
  CoordType _x, _y;
};

std::vector<std::pair<size_t, size_t>>
FindMST(const std::vector<Point2D> &pts) {
  size_t ptCount = pts.size();
  std::vector<bool> used(ptCount, false);
  used[0] = true;
  std::priority_queue<std::pair<Point2D::CoordType, std::pair<size_t, size_t>>>
      q;
  for (size_t i = 1; i < ptCount; ++i) {
    q.push({-pts[0].Distance(pts[i]), {0, i}});
  }
  std::vector<std::pair<size_t, size_t>> mst;
  while (!q.empty()) {
    auto edge = q.top().second;
    q.pop();
    if (used[edge.second]) {
      continue;
    }
    used[edge.second] = true;
    mst.push_back(edge);
    for (size_t i = 0; i < ptCount; ++i) {
      if (!used[i]) {
        q.push({-pts[edge.second].Distance(pts[i]), {edge.second, i}});
      }
    }
  }
  return mst;
}

using Graph = std::vector<std::vector<size_t>>;

void Dfs(const Graph &g, size_t vertex, std::vector<bool> &used,
         std::vector<size_t> &path) {
  path.push_back(vertex);
  used[vertex] = true;
  for (auto to : g[vertex]) {
    if (!used[to]) {
      Dfs(g, to, used, path);
    }
  }
}

Point2D::CoordType ComputeTourDistance(const std::vector<Point2D> &pts,
                                       const std::vector<size_t> &tour) {
  Point2D::CoordType distance = 0;
  for (size_t i = 0; i < tour.size(); ++i) {
    distance += pts[tour[i]].Distance(pts[tour[(i + 1) % tour.size()]]);
  }
  return distance;
}

std::vector<size_t> GetCycle(const Graph &g, const std::vector<Point2D> &pts) {
  size_t nodesCount = g.size();
  std::vector<size_t> path;
  std::vector<bool> used(nodesCount, false);
  std::vector<size_t> bestCycle;
  Point2D::CoordType bestDistance = 0;
  for (size_t i = 0; i < nodesCount; ++i) {
    path.clear();
    std::fill(used.begin(), used.end(), false);
    Dfs(g, i, used, path);
    auto currentDistance = ComputeTourDistance(pts, path);
    if (!i || bestDistance > currentDistance) {
      bestCycle = path;
      bestDistance = currentDistance;
    }
  }
  return bestCycle;
}

using Solution = std::pair<Point2D::CoordType, std::vector<size_t>>;

Solution MSTSolution(const std::vector<Point2D> &pts) {
  size_t ptCount = pts.size();
  auto mst = FindMST(pts);
  Graph tree(ptCount);
  for (const auto &e : mst) {
    tree[e.first].push_back(e.second);
    tree[e.second].push_back(e.first);
  }
  auto cycle = GetCycle(tree, pts);
  return {ComputeTourDistance(pts, cycle), cycle};
}

Solution LocalSearchSolution(const std::vector<Point2D> &pts) {
  auto init = MSTSolution(pts);
  auto bestDistance = init.first;
  std::cerr << "Init distance: " << bestDistance << std::endl;
  auto bestSolution = init.second;
  auto currentDistance = init.first;
  auto currentSolution = init.second;
  std::uniform_real_distribution<long double> unif(0, 1);
  std::uniform_int_distribution<size_t> ind(0, pts.size() - 1);
  std::random_device rand_dev;
  std::mt19937 rand_engine(rand_dev());
  long double temp = 5000;
  for (size_t it = 0; it < 6000000; ++it) {
    size_t l = ind(rand_engine);
    size_t r = ind(rand_engine);
    if (l > r) {
      std::swap(l, r);
    }
    auto next = currentSolution;
    std::reverse(next.begin() + l, next.begin() + r + 1);
    Point2D::CoordType nextDistance = ComputeTourDistance(pts, next);
    auto delta = (nextDistance - currentDistance) / nextDistance;
    auto prob = std::exp(-delta / temp);
    if (nextDistance < currentDistance || unif(rand_engine) < prob) {
      currentDistance = nextDistance;
      currentSolution = next;
      if (currentDistance < bestDistance) {
        std::cerr << "New distance found: " << currentDistance << '\r';
        bestSolution = currentSolution;
        bestDistance = currentDistance;
      } else {
        temp *= 0.99996;
      }
    }
  }
  return {bestDistance, bestSolution};
}

Solution BruteForceSolution(const std::vector<Point2D> &pts) {
  size_t ptCount = pts.size();
  std::vector<size_t> perm(ptCount);
  std::iota(perm.begin(), perm.end(), 0);
  std::vector<size_t> bestSolution = perm;
  Point2D::CoordType bestDistance = ComputeTourDistance(pts, perm);
  while (std::next_permutation(perm.begin(), perm.end())) {
    auto currentDistance = ComputeTourDistance(pts, perm);
    if (currentDistance < bestDistance) {
      bestSolution = perm;
      bestDistance = currentDistance;
    }
  }
  return {bestDistance, bestSolution};
}

void solve(std::istream &in, std::ostream &out) {
  size_t ptCount;
  in >> ptCount;
  std::vector<Point2D> pts(ptCount);
  std::unordered_map<size_t, int64_t> idByInd;
  for (size_t i = 0; i < ptCount; ++i) {
    int64_t id = i;
    in >> pts[i];
    idByInd[i] = id;
  }
  Solution solution;
  if (ptCount <= 12) {
    solution = BruteForceSolution(pts);
  } else {
    solution = LocalSearchSolution(pts);
  }
  for (auto v : solution.second) {
    out << idByInd[v] << ' ';
  }
  out << idByInd[solution.second[0]];
}

int main() {
  solve(std::cin, std::cout);
  return EXIT_SUCCESS;
}