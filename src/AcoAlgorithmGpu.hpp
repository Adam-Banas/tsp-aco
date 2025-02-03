#include "AcoAlgorithm.hpp"

#ifndef ACO_ALGORITHM_GPU_HPP
#define ACO_ALGORITHM_GPU_HPP

namespace aco {

// CUDA implmentation of the ACO algorithm.
class AlgorithmGpu : public Algorithm {
  public:
    friend class Algorithm;

  private:
    // Should be created via factory method.
    explicit AlgorithmGpu(std::mt19937& random_generator, Graph graph, Config config);

  public:
    // Accessors. These may trigger synchronization and data transfer between GPU and host.
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

#endif // ACO_ALGORITHM_GPU_HPP