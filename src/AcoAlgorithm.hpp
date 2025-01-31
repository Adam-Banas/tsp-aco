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
class Algorithm {
  public:
    using Path = std::vector<Graph::Index>;

    // Algorithm configuration
    struct Config {
        std::size_t agents_count; // The number of agents per iteration (between pheromone updates)
        float       pheromone_evaporation; // Pheromone evaporation coefficient: in [0,1] range:
                                           // * 1 means no evaporation (100% pheromones remain)
                                           // * 0 means full evaporation (0% pheromones remain)
    };

  public:
    explicit Algorithm(std::mt19937& random_generator, Graph graph, Config config);

  public:
    // Accessors
    const Graph& get_graph() const;
    const Path&  get_shortest_path() const;

    // Advance simulation by one step
    void advance();

  private:
    // TODO: Move the following method to aco::Graph
    int path_length(const Path& path) const;

  private:
    std::mt19937& gen;
    Graph         graph;
    Config        config;
    Path          shortest_path;
};

} // namespace aco

#endif // ACO_ALGORITHM_HPP