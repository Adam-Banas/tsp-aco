#include "AcoAlgorithm.hpp"

namespace aco {
AcoAlgorithm::AcoAlgorithm(std::unique_ptr<Graph> graph, Config config) {}
const Graph&              AcoAlgorithm::get_graph() const {}
const AcoAlgorithm::Path& AcoAlgorithm::get_shortest_path() const {}
void                      AcoAlgorithm::advance() {}
} // namespace aco