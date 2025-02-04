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

std::ostream& operator<<(std::ostream& out, const std::vector<aco::Graph::Index>& path) {
    out << "[";
    for (auto elem : path) {
        out << elem << " ";
    }
    out << "]";
    return out;
}

void standard_simulation(int max_iterations, std::mt19937& gen, aco::Graph graph,
                         aco::Algorithm::Config config) {
    std::vector<std::unique_ptr<aco::Algorithm>> algorithms;
    algorithms.push_back(aco::Algorithm::make(aco::DeviceType::CPU, gen, graph, config));
    algorithms.push_back(aco::Algorithm::make(aco::DeviceType::CPU, gen, graph, config));
    algorithms.push_back(aco::Algorithm::make(aco::DeviceType::CPU, gen, graph, config));
    algorithms.push_back(aco::Algorithm::make(aco::DeviceType::GPU, gen, graph, config));
    algorithms.push_back(aco::Algorithm::make(aco::DeviceType::GPU, gen, graph, config));
    algorithms.push_back(aco::Algorithm::make(aco::DeviceType::GPU, gen, graph, config));

    // Main loop
    for (int i = 0; i < max_iterations; ++i) {
        std::cout << "\n\nIteration " << i << "\n\n";
        for (auto& algorithm : algorithms) {
            // Remember previous best, advance simulation
            auto previous_best = algorithm->get_shortest_path();
            auto iteration_best = algorithm->advance();

            // Print results
            std::cout << "Algorithm: " << algorithm->info() << "\n";
            std::cout << "Shortest path: " << iteration_best
                      << ", length: " << algorithm->path_length(iteration_best) << "\n";
            std::cout << "Previous best: " << previous_best
                      << ", length: " << algorithm->path_length(previous_best) << "\n";
        }
    }
}

void benchmark_simulation(int max_iterations, std::mt19937& gen, aco::Graph graph,
                          aco::Algorithm::Config config) {
    std::vector<std::unique_ptr<aco::Algorithm>> algorithms;
    algorithms.push_back(aco::Algorithm::make(aco::DeviceType::CPU, gen, graph, config));
    algorithms.push_back(aco::Algorithm::make(aco::DeviceType::GPU, gen, graph, config));

    struct Result {
        std::string      info;
        int              best_path_length;
        std::vector<int> iteration_times;
        int              total_time;
    };
    std::vector<Result> results;

    // Main loop
    for (auto& algorithm : algorithms) {
        Result algorithm_result;
        std::cout << "Starting simulation using algorithm: " << algorithm->info() << "...\n";
        algorithm_result.iteration_times.resize(max_iterations);
        auto begin = std::chrono::steady_clock::now();
        for (int i = 0; i < max_iterations; ++i) {
            auto iter_begin = std::chrono::steady_clock::now();
            auto iteration_best = algorithm->advance();
            auto iter_end = std::chrono::steady_clock::now();
            algorithm_result.iteration_times[i] =
                std::chrono::duration_cast<std::chrono::milliseconds>(iter_end - iter_begin)
                    .count();
            std::cout << "Iteration time: " << algorithm_result.iteration_times[i]
                      << " ms, path length: " << algorithm->path_length(iteration_best) << "\n";
        }
        // TODO: Synchronize
        auto end = std::chrono::steady_clock::now();
        algorithm_result.total_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();

        algorithm_result.info = algorithm->info();
        algorithm_result.best_path_length = algorithm->path_length(algorithm->get_shortest_path());

        std::cout << "Total time: " << algorithm_result.total_time << " ms.\n";
        results.push_back(std::move(algorithm_result));
    }

    std::cout << "\n\nAll simulation finished, summary:\n";
    for (auto elem : results) {
        std::cout << "Algorithm: " << elem.info << "\n";
        std::cout << "Final path length: " << elem.best_path_length << "\n";
        std::cout << "Total time: " << elem.total_time << " ms\n";
        std::cout << "Best iteration: "
                  << *std::min_element(begin(elem.iteration_times), end(elem.iteration_times))
                  << " ms\n";
        std::cout << "Worst iteration: "
                  << *std::max_element(begin(elem.iteration_times), end(elem.iteration_times))
                  << " ms\n";
    }
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    if (argc != 2) {
        std::cout << "Ant Colony Optimization algorithm applied to the Travelling Salesman "
                     "Problem.\n";
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
    aco::Graph         graph = aco::Graph(gen, cities, min_pheromone);

    aco::Algorithm::Config config = {.agents_count = agents,
                                     .pheromone_evaporation = pheromone_evaporation};

    bool benchmark = true;
    if (!benchmark) {
        standard_simulation(max_iterations, gen, graph, config);
    } else {
        benchmark_simulation(max_iterations, gen, graph, config);
    }
}