#include "AcoAlgorithm.hpp"

#include <iostream>

#include "utils.hpp"

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
    : gen(random_generator), graph(std::move(graph_arg)), config(config_arg), shortest_path() {
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

void Algorithm::advance() {
    auto cities = graph.get_size();

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
        if (path_length(path) < path_length(shortest_path)) {
            shortest_path = path;
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
}

int Algorithm::path_length(const Path& path) const {
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