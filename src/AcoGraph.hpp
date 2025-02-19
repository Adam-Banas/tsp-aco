#include <memory>
#include <random>
#include <vector>

#ifndef ACO_GRAPH_HPP
#define ACO_GRAPH_HPP

namespace aco {

// Asymmetric graph representing the problem domain along with algorithm-specific behavior.
// Cost is the cost from going from one node to the other (e.g. distance).
// Graph-wide behavior:
// - Every function that takes index or indices, throw when one of them is out-of-range.
// - Every function that takes two indices (as src and dst, or two-way), throws when they have the
//   same value. In other words, can't determine the cost or pheromone amount on an edge to self,
//   because such an edge does not exist.
// - It is not possible to go under initial pheromone level - if a new value would be set to below
//   initial level, it set to initial level instead.
class Graph {
  public:
    using Index = std::size_t;

    friend bool operator==(const Graph&, const Graph&);
    friend class AlgorithmGpu;

  public:
    // Create a graph with a given number of nodes.
    // Currently there's only one, implicit initialization method:
    // - full graph,
    // - initialize all cost edges using uniform distribution, but symmetrically
    // - all pheromones get the same amount of initial pheromone
    explicit Graph(std::mt19937& random_generator, std::size_t nodes, float initial_pheromone);

  private:
    // Value constructor, useful for testing
    explicit Graph(std::vector<int> costs, std::vector<float> pheromones, std::size_t nodes,
                   float initial_pheromone);

  public:
    // Basic graph operations
    std::size_t get_size() const;
    int         get_cost(Index src, Index dst) const;
    float       get_pheromone(Index src, Index dst) const;
    void        set_pheromone(Index src, Index dst, float value);

    // Convenience functions
    void add_pheromone_two_way(Index a, Index b, float amount);
    void update_all(float coefficient);

    // Serialization. The idea here is to serialize to a human-readable format, not really for
    // efficiency.
    std::string  to_string() const;
    static Graph from_string(const std::string& string);

  private:
    Index internal_index(Index src, Index dst) const;

  private:
    std::vector<int>   costs;
    std::vector<float> pheromones;
    std::size_t        nodes;
    float              initial_pheromone;
};

bool        operator==(const Graph& lhs, const Graph& rhs);
inline bool operator!=(const Graph& lhs, const Graph& rhs) {
    return !(lhs == rhs);
}

} // namespace aco

#endif // ACO_GRAPH_HPP