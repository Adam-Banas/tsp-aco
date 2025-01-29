#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include "AcoAlgorithm.hpp"
#include "utils.hpp"

using Graph = std::vector<std::vector<int>>;
using FGraph = std::vector<std::vector<float>>;
using Path = std::vector<std::size_t>; // Indices of cities in order

// A symmetric graph, with arbitrary numbers as distances
Graph make_distances_graph(int cities, std::mt19937& gen) {
    Graph edges(cities, std::vector<int>(cities));

    std::uniform_int_distribution<> distrib(1, /*max_dist=*/10);

    // Populate graph
    for (int i = 0; i < cities; ++i) {
        for (int j = i + 1; j < cities; ++j) {
            auto dist = distrib(gen);
            edges.at(i).at(j) = dist;
            edges.at(j).at(i) = dist;
        }
    }

    return edges;
}

FGraph make_pheromones_graph(int cities, float min_pheromone) {
    return FGraph(cities, std::vector<float>(cities, min_pheromone));
}

auto path_length(const Graph& graph, const Path& path) {
    int length = 0;
    for (int i = 0; i < path.size(); ++i) {
        // Path stores visited cities in order. It is a round trip, so the last distance is from the
        // last city directly to the first one
        auto src = path[i];
        auto dst = path[(i + 1) % path.size()];

        length += graph[src][dst];
    }

    return length;
}

const Path& get_shortest_path(const Graph& graph, const std::vector<Path>& paths) {
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
    int   cities = 10;
    int   agents = cities;
    float min_pheromone = 1;
    float pheromone_evaporation = 0.9; // 10% of pheromone evaporates every iteration

    // Initialization
    std::random_device rd;
    std::mt19937       gen(rd());
    auto               distances = make_distances_graph(cities, gen);
    auto               pheromones = make_pheromones_graph(cities, min_pheromone);

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
                    path_scores[j] = pheromones[current_city][j] / distances[current_city][j];
                }

                // Choose the target city using roullette random algorithm
                auto target = utils::roullette(path_scores, gen);
                path.push_back(target);
            }

            //
        }

        // TODO: Update pheromones

        // Get the shortest path from the agents, remember if shorter than shortest so far
        auto current_shortest = get_shortest_path(distances, paths);
        std::cout << "\nIteration " << a << " results:\n";
        std::cout << "Shortest path length: " << path_length(distances, current_shortest) << "\n";
        std::cout << "Best so far: " << path_length(distances, shortest_path) << "\n";
        if (path_length(distances, current_shortest) < path_length(distances, shortest_path)) {
            shortest_path = current_shortest;
        }
    }
}