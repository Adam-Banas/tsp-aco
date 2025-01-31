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
    : graph(std::move(graph_arg)), config(config_arg) {
    validate_config(config);
}

const Graph&           Algorithm::get_graph() const {}
const Algorithm::Path& Algorithm::get_shortest_path() const {}
void                   Algorithm::advance() {}
} // namespace aco