// #include <algorithm>
// #include <vector>
#include <memory>

#include "AcoGraph.hpp"

#ifndef ACO_ALGORITHM_HPP
#define ACO_ALGORITHM_HPP

namespace aco {

// class MoveStrategy;

// Main algorithm that uses ACO (Ant Colony Optimization) to solve a graph problem.
// At the moment it is tightly coupled to solve TSP (Travelling Salesman Problem).
class AcoAlgorithm {
  public:
    using Path = std::vector<Graph::Index>;

    // Algorithm configuration
    struct Config {
        int   agents_count;          // The number of agents per iteration (before pheromone update)
        float min_pheromone;         // Pheromone amount will never go under this value
        float pheromone_evaporation; // Pheromone evaporation coefficient: in [0,1] range:
                                     // * 1 means no evaporation (100% pheromones remain)
                                     // * 0 means full evaporation (0% pheromones remain)
    };

  public:
    explicit AcoAlgorithm(std::unique_ptr<Graph> graph, Config config);

  public:
    // Accessors
    const Graph& get_graph() const;
    const Path&  get_shortest_path() const;

    // Advance simulation by one step
    void advance();

  private:
    std::unique_ptr<Graph> graph;
    Config                 config;
    Path                   shortest_path;
};

} // namespace aco

#endif // ACO_ALGORITHM_HPP