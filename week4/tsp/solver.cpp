#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class Vector {
 public:
  Vector() : x(0), y(0) {}
  Vector(double x, double y) : x(x), y(y) {}

  friend std::istream& operator>>(std::istream& in, Vector& v) {
    double x, y;
    in >> x >> y;
    v = Vector(x, y);
    return in;
  }

  double Length() const { return std::sqrt(x * x + y * y); }

  friend Vector operator-(const Vector& lhs, const Vector& rhs) {
    return Vector(lhs.x - rhs.x, lhs.y - rhs.y);
  }

 private:
  double x, y;
};

struct Solution {
  double distance = 0;
  bool isOptimal = false;
  std::vector<size_t> indices;

  friend std::ostream& operator<<(std::ostream& out, const Solution& solution) {
    out << solution.distance << ' ' << solution.isOptimal << '\n';
    for (auto i : solution.indices) {
      out << i << ' ';
    }
    return out;
  }
};

Solution GreedySolution(const std::vector<Vector>& pts) {
  Solution s;
  s.indices.push_back(0);
  size_t ptCount = pts.size();
  std::vector<bool> used(ptCount, false);
  used[0] = true;
  while (s.indices.size() != ptCount) {
    size_t last = s.indices.back();
    size_t next = last;
    for (size_t i = 0; i < ptCount; ++i) {
      if (!used[i] && (next == last || (pts[next] - pts[last]).Length() >
                                           (pts[i] - pts[last]).Length())) {
        next = i;
      }
    }
    s.distance += (pts[next] - pts[last]).Length();
    s.indices.push_back(next);
    used[next] = true;
  }
  return s;
}

void solve(std::istream& in, std::ostream& out) {
  size_t ptCount;
  in >> ptCount;
  std::vector<Vector> pts(ptCount);
  for (auto& p : pts) {
    in >> p;
  }
  out << GreedySolution(pts);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    throw std::runtime_error("Usage: ./" + std::string(argv[0]) + " <filename>");
  }
  std::ifstream fin(argv[1]);
  solve(fin, std::cout);
  return EXIT_SUCCESS;
}