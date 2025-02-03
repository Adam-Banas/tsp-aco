#include "AcoAlgorithm.hpp"

#include <iostream>

#include "AcoAlgorithmCpu.hpp"
#include "AcoAlgorithmGpu.hpp"

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

// Factory method
std::unique_ptr<Algorithm> Algorithm::make(DeviceType device, std::mt19937& random_generator,
                                           Graph graph, Config config) {
    switch (device) {
    case DeviceType::CPU:
        return std::unique_ptr<Algorithm>(
            new AlgorithmCpu(random_generator, std::move(graph), config));
    case DeviceType::GPU:
        return std::unique_ptr<Algorithm>(
            new AlgorithmGpu(random_generator, std::move(graph), config));
    }

    std::cerr << "aco::Algorithm::make_algorithm unknown device type: " << (int)device << "\n";
    throw std::invalid_argument("aco::Algorithm::make_algorithm unknown device type");
}

} // namespace aco