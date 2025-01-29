// #include <algorithm>
#include <memory>
#include <vector>

#ifndef ACO_GRAPH_HPP
#define ACO_GRAPH_HPP

namespace aco {

// Asymmetric graph representing the problem domain along with algorithm-specific behavior.
// Cost is the cost from going from one node to the other (e.g. distance).
class Graph {
  public:
    using Index = std::size_t;

  public:
    // Create a graph with a given number of nodes.
    // Currently there's only one, implicit initialization method:
    // - full graph,
    // - initialize all cost edges using uniform distribution, but symmetrically
    // - all pheromones get the same amount of initial pheromone
    explicit Graph(int nodes, float initial_pheromone);

  public:
    // Basic graph manipulation
    int   get_cost(Index src, Index dst) const;
    float get_pheromone(Index src, Index dst) const;
    float set_pheromone(Index src, Index dst, float value);

    // Convenience functions
    void add_pheromone_two_way(Index a, Index b, float amount);
    void update_all(float coefficient);

  private:
    std::vector<int>   costs;
    std::vector<float> pheromones;
};

} // namespace aco

#endif // ACO_GRAPH_HPP