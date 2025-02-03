#include <cstddef>
#include <memory>
#include <random>
#include <vector>

#include "AcoGraph.hpp"

#ifndef ACO_ALGORITHM_HPP
#define ACO_ALGORITHM_HPP

namespace aco {

// Base class for algorithms that use ACO (Ant Colony Optimization) to solve a graph problem.
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
    // Throws std::invalid_argument on invalid configuration
    explicit Algorithm(std::mt19937& random_generator, Graph graph, Config config);
    virtual ~Algorithm() = default;

  public:
    // Accessors. For algorithms operating on GPU, this is also synchronization point.
    virtual const Graph& get_graph() const = 0;
    virtual const Path&  get_shortest_path() const = 0;

    // Advance simulation by one step. Return best path from that iteration.
    virtual Path advance() = 0;

  public:
    // TODO: Move the following method to aco::Graph
    virtual int path_length(const Path& path) const = 0;

  protected:
    std::mt19937& gen;
    Graph         graph;
    Config        config;
};

} // namespace aco

#endif // ACO_ALGORITHM_HPP