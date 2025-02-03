#include <memory>

#include "AcoAlgorithm.hpp"

#ifndef ACO_ALGORITHM_CPU_HPP
#define ACO_ALGORITHM_CPU_HPP

namespace aco {

// Main algorithm that uses ACO (Ant Colony Optimization) to solve a graph problem.
// At the moment it is tightly coupled to solve TSP (Travelling Salesman Problem).
class AlgorithmCpu : public Algorithm {
  public:
    explicit AlgorithmCpu(std::mt19937& random_generator, Graph graph, Config config);

  public:
    // Accessors
    const Graph& get_graph() const override;
    const Path&  get_shortest_path() const override;

    // Advance simulation by one step. Return best path from that iteration.
    Path advance() override;

  public:
    // TODO: Move the following method to aco::Graph
    int path_length(const Path& path) const override;

  private:
    Path shortest_path;
};

} // namespace aco

#endif // ACO_ALGORITHM_CPU_HPP