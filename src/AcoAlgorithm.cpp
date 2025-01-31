#include "AcoAlgorithm.hpp"

#include <iostream>

namespace aco {

static void validate_config(Algorithm::Config config) {
    if (config.agents_count == 0) {
        std::cerr << "aco::Algorithm invalid argument. Agent count should be non-zero!\n";
        throw std::invalid_argument("aco::Algorithm invalid agents count argument!");
    }
    if (config.pheromone_evaporation < 0 || config.pheromone_evaporation > 1) {
        std::cerr << "aco::Algorithm invalid pheromone evaporation argument! Expected in range "
                     "[0,1], got: "
                  << config.pheromone_evaporation << "\n";
        throw std::invalid_argument("aco::Algorithm invalid pheromone evaporation argument!");
    }
}

Algorithm::Algorithm(Graph graph_arg, Config config_arg)
    : graph(std::move(graph_arg)), config(config_arg), shortest_path() {
    // Validate arguments
    validate_config(config);

    // Initialize shortest path just to be valid
    shortest_path.resize(graph.get_size());
    std::iota(begin(shortest_path), end(shortest_path), 0);
}

const Graph& Algorithm::get_graph() const {
    return graph;
}

const Algorithm::Path& Algorithm::get_shortest_path() const {
    return shortest_path;
}

void Algorithm::advance() {}
} // namespace aco