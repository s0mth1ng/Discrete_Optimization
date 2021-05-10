#include <pthread.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <vector>

class Vector {
   public:
    using CoordType = long double;
    Vector() : x(0), y(0) {}
    Vector(CoordType x, CoordType y) : x(x), y(y) {}

    friend std::istream& operator>>(std::istream& in, Vector& v) {
        CoordType x, y;
        in >> x >> y;
        v = Vector(x, y);
        return in;
    }

    CoordType Length() const { return std::sqrt(x * x + y * y); }

    static CoordType ComputeDistance(const Vector& v1, const Vector& v2) {
        return (v1 - v2).Length();
    }

    friend Vector operator-(const Vector& lhs, const Vector& rhs) {
        return Vector(lhs.x - rhs.x, lhs.y - rhs.y);
    }

   private:
    CoordType x, y;
};

class StopWatch {
   public:
    void Start() {
        if (_started) {
            throw std::runtime_error("Watch are already running");
        }
        _started = true;
        _start = std::chrono::system_clock::now();
    }

    void Reset() { _started = false; }

    size_t GetDurationInMilliseconds() const {
        auto end = std::chrono::system_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - _start);
        return duration.count();
    }

   private:
    std::chrono::time_point<std::chrono::system_clock> _start;
    bool _started = false;
};

class LocalSearchSolver {
   public:
    struct Solution {
        Vector::CoordType distance = 0;
        bool isOptimal = false;
        std::vector<size_t> indices;

        friend std::ostream& operator<<(std::ostream& out,
                                        const Solution& solution) {
            out << std::fixed << solution.distance << ' ' << solution.isOptimal
                << '\n';
            for (auto i : solution.indices) {
                out << i << ' ';
            }
            return out;
        }
    };

    LocalSearchSolver(const std::vector<Vector>& pts) : _pts(pts) {}

    Solution FindSolution(size_t maxTimeInSeconds = 60 * 10) const {
        Solution current = GreedySolution();
        Solution best = current;
        std::cerr << "Greedy solution found. Distance: " << std::fixed
                  << current.distance << std::endl;
        StopWatch watch;
        watch.Start();
        bool running = true;
        std::uniform_real_distribution<long double> unif_prob(0, 1);
        std::uniform_int_distribution<size_t> unif_ind(0, _pts.size() - 1);
        std::default_random_engine re;
        const long double INIT_TEMP = _pts.size() * 500;
        long double temp = INIT_TEMP;
        long double alpha = 0.994;
        size_t it = 0;
        while (running) {
            size_t e1 = unif_ind(re);
            size_t e2 = unif_ind(re);
            if (e1 > e2) {
                std::swap(e1, e2);
            }
            if (e1 == e2 || (!e1 && e2 + 1 == _pts.size())) {
                continue;
            }
            Vector::CoordType diff =
                ComputeDifferenceAfterSwap(current, e1, e2);
            long double prob = std::exp(diff / temp);
            if (diff > 0 || unif_prob(re) < prob) {
                current = MakeSwap(current, e1, e2);
                current.distance -= diff;
                if (current.distance < best.distance) {
                    best = current;
                    std::cerr << "New distance found: " << std::fixed
                              << best.distance << '\r';
                }
                it++;
                temp = INIT_TEMP * alpha / it;
            }
            size_t duration = watch.GetDurationInMilliseconds();
            running = (duration < 1000 * maxTimeInSeconds);
        }
        return current;
    }

   private:
    const Vector::CoordType EPS = 1e-6;

    std::vector<Vector> _pts;

    Solution GreedySolution() const {
        Solution s;
        s.indices.push_back(0);
        size_t ptCount = _pts.size();
        std::vector<bool> used(ptCount, false);
        used[0] = true;
        while (s.indices.size() != ptCount) {
            size_t last = s.indices.back();
            size_t next = last;
            for (size_t i = 0; i < ptCount; ++i) {
                if (!used[i] &&
                    (next == last ||
                     ComputeDistance(next, last) > ComputeDistance(i, last))) {
                    next = i;
                }
            }
            s.indices.push_back(next);
            used[next] = true;
        }
        s.distance = ComputeTourDistance(s.indices);
        return s;
    }

    Vector::CoordType ComputeTourDistance(
        const std::vector<size_t>& indices) const {
        Vector::CoordType length = 0;
        for (size_t i = 0; i < indices.size(); ++i) {
            length +=
                ComputeDistance(indices[i], indices[(i + 1) % _pts.size()]);
        }
        return length;
    }

    Vector::CoordType ComputeDistance(size_t p1, size_t p2) const {
        return Vector::ComputeDistance(_pts[p1 % _pts.size()],
                                       _pts[p2 % _pts.size()]);
    }

    Vector::CoordType ComputeDifferenceAfterSwap(const Solution& solution,
                                                 size_t e1,
                                                 size_t e2) const {
        size_t B = solution.indices[(e1 + _pts.size() - 1) % _pts.size()];
        size_t C = solution.indices[e1];
        size_t F = solution.indices[(e2 + _pts.size() + 1) % _pts.size()];
        size_t E = solution.indices[e2];
        Vector::CoordType length =
            ComputeDistance(B, C) + ComputeDistance(E, F);
        length -= ComputeDistance(B, E) + ComputeDistance(C, F);
        return length;
    }

    Solution MakeSwap(const Solution& solution, size_t e1, size_t e2) const {
        if (e1 > e2) {
            std::swap(e1, e2);
        }
        Solution newSolution = solution;
        std::reverse(newSolution.indices.begin() + e1,
                     newSolution.indices.begin() + e2 + 1);
        return newSolution;
    }
};