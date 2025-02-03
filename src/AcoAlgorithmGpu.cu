#include "AcoAlgorithmGpu.hpp"

#include <iostream>

#include "Utils.hpp"

namespace aco {

// Initialize shortest path just to be valid
static auto make_valid_path(const Graph& graph) {
    Algorithm::Path result(graph.get_size());
    std::iota(begin(result), end(result), 0);
    return result;
}

AlgorithmGpu::AlgorithmGpu(std::mt19937& random_generator, Graph graph_arg, Config config_arg)
    : Algorithm(random_generator, std::move(graph_arg), config_arg), shortest_path() {
    shortest_path = make_valid_path(graph);
}

const Graph& AlgorithmGpu::get_graph() const {
    return graph;
}

const AlgorithmGpu::Path& AlgorithmGpu::get_shortest_path() const {
    return shortest_path;
}

AlgorithmGpu::Path AlgorithmGpu::advance() {
    auto cities = graph.get_size();
    Path iteration_best = make_valid_path(graph);

    // Generate solutions
    std::vector<Path> paths(config.agents_count);
    for (std::size_t i = 0; i < config.agents_count; ++i) {
        auto& path = paths[i];

        // Start from a city with index 'i', modulo in case the number of agents is higher than the
        // number of cities
        path.push_back(i % cities);

        // Choose one new destination in every iteration
        while (path.size() < cities) {
            // Calculate the score (desire to go) for every city
            std::vector<float> path_scores(cities);
            auto               current_city = path.back();
            for (std::size_t j = 0; j < cities; ++j) {
                if (utils::contains(path, j)) {
                    // Path already visited - leave it a score of zero
                    continue;
                }

                // Basic score function without alpha and beta coefficients
                // Basic heuristic - just a reciprocal of the distance, so that shorter paths
                // are preferred in general
                // TODO: Precompute reciprocals of distances?
                path_scores[j] =
                    graph.get_pheromone(current_city, j) / graph.get_cost(current_city, j);
            }

            // Choose the target city using roullette random algorithm
            auto target = utils::roullette(path_scores, gen);
            path.push_back(target);
        }

        // Path calculated - remember it if is shorter than the current best
        if (path_length(path) < path_length(iteration_best)) {
            iteration_best = path;
        }
    }

    // Update pheromones
    // Step 1: evaporation
    graph.update_all(config.pheromone_evaporation);

    // Step 2: Pheromones left by ants.
    // Basic algorithm, where every ant leaves pheromones, and the amount is independent from
    // other ants' solutions.
    // No limit on total pheromone on a section.
    for (const auto& path : paths) {
        // The total amount of pheromone left by ant is inversely proportional to the distance
        // covered by ant.
        float total_pheromone = 1.f / path_length(path);

        for (int i = 0; i < path.size(); ++i) {
            // Path stores visited cities in order. It is a round trip, so the last distance is
            // from the last city directly to the first one
            auto src = path[i];
            auto dst = path[(i + 1) % path.size()];

            // The amount of pheromone to leave is proportional to the section length
            float pheromone_to_leave = total_pheromone / graph.get_cost(src, dst);
            graph.add_pheromone_two_way(src, dst, pheromone_to_leave);
        }
    }

    // If the iteration best path is shortest than the global shortest (best so far), remember it
    if (path_length(iteration_best) < path_length(shortest_path)) {
        shortest_path = iteration_best;
    }

    return iteration_best;
}

int AlgorithmGpu::path_length(const Path& path) const {
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

} // namespace aco