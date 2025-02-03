#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include "AcoAlgorithmCpu.hpp"
#include "AcoGraph.hpp"
#include "Utils.hpp"

using Path = std::vector<std::size_t>; // Indices of cities in order

auto path_length(const aco::Graph& graph, const Path& path) {
    int length = 0;
    for (int i = 0; i < path.size(); ++i) {
        // Path stores visited cities in order. It is a round trip, so the last distance is from the
        // last city directly to the first one
        auto src = path[i];
        auto dst = path[(i + 1) % path.size()];

        length += graph.get_cost(src, dst);
    }

    return length;
}

const Path& get_shortest_path(const aco::Graph& graph, const std::vector<Path>& paths) {
    if (paths.empty()) {
        throw std::runtime_error("get_shortest_path: Empty paths container!");
    }

    return *std::min_element(begin(paths), end(paths), [&](const Path& a, const Path& b) {
        return path_length(graph, a) < path_length(graph, b);
    });
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    if (argc != 2) {
        std::cout
            << "Ant Colony Optimization algorithm applied to the Travelling Salesman Problem.\n";
        std::cout << "Usage: " << argv[0] << " iterations\n";
        return 1;
    }
    int max_iterations = std::stoi(argv[1]);

    // Configuration
    std::size_t cities = 64;
    std::size_t agents = cities * 16;
    float       pheromone_evaporation = 0.9; // 10% of pheromone evaporates every iteration
    float       min_pheromone = 0.1; // Minimum pheromone on an edge (which is also the starting
                                     // pheromone). I observed the following rough guidelines:
    // - The greater total distance to travel (depends on the number of cities and the distances
    //   between them), the smaller this number needs to be.
    // - The more ants we have, the bigger this number can be.

    // Initialization
    std::random_device rd;
    std::mt19937       gen(rd());

    aco::Algorithm::Config config = {.agents_count = agents,
                                     .pheromone_evaporation = pheromone_evaporation};
    auto                   algorithm = aco::Algorithm::make(aco::DeviceType::GPU, gen,
                                                            aco::Graph(gen, cities, min_pheromone), config);

    // Main loop
    for (int a = 0; a < max_iterations; ++a) {
        // Remember previous best, advance simulation
        auto previous_best = algorithm->get_shortest_path();
        auto iteration_best = algorithm->advance();

        // Print results
        std::cout << "\nIteration " << a << " results:\n";
        std::cout << "Shortest path length: " << algorithm->path_length(iteration_best) << "\n";
        std::cout << "Previous best: " << algorithm->path_length(previous_best) << "\n";
    }
}