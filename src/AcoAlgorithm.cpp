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

Algorithm::Algorithm(std::mt19937& random_generator, Graph graph_arg, Config config_arg)
    : gen(random_generator), graph(std::move(graph_arg)), config(config_arg) {
    // Validate arguments
    validate_config(config);
}

} // namespace aco