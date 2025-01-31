#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include "AcoGraph.hpp"
#include "utils.hpp"

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
    int   cities = 30;
    int   agents = cities;
    float min_pheromone = 1;
    float pheromone_evaporation = 0.9; // 10% of pheromone evaporates every iteration
    float ant_pheromone_coeff = 10;

    // Initialization
    std::random_device rd;
    std::mt19937       gen(rd());
    auto               graph = aco::Graph(gen, cities, min_pheromone);

    Path shortest_path(cities);
    std::iota(begin(shortest_path), end(shortest_path), 0);

    // Main loop
    for (int a = 0; a < max_iterations; ++a) {
        if (a % 100000 == 0) {
            std::cout << "Starting iteration number " << a << std::endl;
        }

        // Generate solutions
        std::vector<Path> paths(agents);
        for (std::size_t i = 0; i < agents; ++i) {
            // One agent iteration
            auto& path = paths[i];
            path.push_back(i % cities); // Start from a city with index 'i', modulo in case the
                                        // number of agents is higher than the number of cities

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
        }

        // Update pheromones
        // Step 1: evaporation
        graph.update_all(pheromone_evaporation);

        // Step 2: Pheromones left by ants.
        // Basic algorithm, where every ant leaves pheromones, and the amount is independent from
        // other ants' solutions.
        // No limit on total pheromone on a section.
        for (const auto& path : paths) {
            // The total amount of pheromone left by ant is inversely proportional to the distance
            // covered by ant.
            auto  length = path_length(graph, path);
            float total_pheromone = ant_pheromone_coeff / length;

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

        // Get the shortest path from the agents, remember if shorter than shortest so far
        auto current_shortest = get_shortest_path(graph, paths);
        std::cout << "\nIteration " << a << " results:\n";
        std::cout << "Shortest path length: " << path_length(graph, current_shortest) << "\n";
        std::cout << "Best so far: " << path_length(graph, shortest_path) << "\n";
        if (path_length(graph, current_shortest) < path_length(graph, shortest_path)) {
            shortest_path = current_shortest;
        }
    }
}