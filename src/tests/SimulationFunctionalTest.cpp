#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <utility>
#include <vector>

#include "../AcoAlgorithm.hpp"
#include "../AcoGraph.hpp"

using aco::Algorithm;
using aco::DeviceType;
using aco::Graph;

static Graph load_graph(const std::string filename) {
    // Read file
    if (!std::filesystem::exists(filename)) {
        std::cerr << "Error: Could not open the file: " << filename << "\n";
        throw std::runtime_error("Could not open the file!");
    }
    std::ifstream file(filename);
    std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // Create graph
    return Graph::from_string(contents);
}

class SimulationFunctionalTest : public ::testing::Test {
  public:
    SimulationFunctionalTest() : gen(/*seed=*/42) {}

  public:
    // Convenience function, to avoid passing common arguments every time
    auto make_algorithm(aco::Graph graph, Algorithm::Config config, DeviceType device) {
        return Algorithm::make(device, gen, std::move(graph), config);
    }

  public:
    std::mt19937 gen;
};

static float last_20_average(const std::vector<int>& in) {
    auto sum = std::accumulate(end(in) - 20, end(in), 0);
    return sum / 20.f;
}

static double sum_pheromones(const Graph& graph) {
    double sum = 0;
    for (std::size_t i = 0; i < graph.get_size(); ++i) {
        for (std::size_t j = 0; j < graph.get_size(); ++j) {
            if (i == j) {
                continue;
            }

            sum += graph.get_pheromone(i, j);
        }
    }
    return sum;
}

// The intention of this test is to simply find if nothing is seriously wrong with the simulation.
TEST_F(SimulationFunctionalTest, CompareCpuAndGpuResults) {
    auto       graph = load_graph("graph_64.json");
    const auto nodes = graph.get_size();

    const Algorithm::Config config{/*agents_count=*/nodes * 16, /*pheromone_evaporation=*/0.9};

    auto cpu_algorithm = make_algorithm(graph, config, DeviceType::CPU);
    auto gpu_algorithm = make_algorithm(graph, config, DeviceType::GPU);

    // Simulation
    const auto         max_iterations = 100;
    std::vector<int>   cpu_iter_bests;
    std::vector<int>   gpu_iter_bests;
    std::vector<Graph> cpu_graphs;
    std::vector<Graph> gpu_graphs;
    for (int i = 0; i < max_iterations; ++i) {
        auto cpu_best = cpu_algorithm->advance();
        cpu_iter_bests.push_back(cpu_algorithm->path_length(cpu_best));
        cpu_graphs.push_back(cpu_algorithm->get_graph());

        auto gpu_best = gpu_algorithm->advance();
        gpu_iter_bests.push_back(gpu_algorithm->path_length(gpu_best));
        gpu_graphs.push_back(gpu_algorithm->get_graph());
    }

    // The average result from the last 20 iterations should be rougly similar between CPU and GPU,
    // also significantly better than found in the first iteration. Limits found empirically.
    auto cpu_last_iters_average = last_20_average(cpu_iter_bests);
    auto cpu_first_iter = cpu_iter_bests.at(0);
    auto gpu_last_iters_average = last_20_average(gpu_iter_bests);
    auto gpu_first_iter = gpu_iter_bests.at(0);

    EXPECT_GT(gpu_last_iters_average, cpu_last_iters_average * 0.9);
    EXPECT_GT(cpu_last_iters_average, gpu_last_iters_average * 0.9);
    EXPECT_LT(cpu_last_iters_average, cpu_first_iter * 0.5);
    EXPECT_LT(gpu_last_iters_average, gpu_first_iter * 0.5);

    std::cout << "cpu_last_iters_average: " << cpu_last_iters_average << "\n";
    std::cout << "cpu_first_iter: " << cpu_first_iter << "\n";
    std::cout << "gpu_last_iters_average: " << gpu_last_iters_average << "\n";
    std::cout << "gpu_first_iter: " << gpu_first_iter << "\n";

    // The sum of pheromones should be roughly similar
    auto cpu_sum_pheromones_final = sum_pheromones(cpu_graphs.back());
    auto cpu_sum_pheromones_initial = sum_pheromones(cpu_graphs.at(0));
    auto gpu_sum_pheromones_final = sum_pheromones(gpu_graphs.back());
    auto gpu_sum_pheromones_initial = sum_pheromones(gpu_graphs.at(0));

    std::cout << "Initial sum of CPU pheromones: " << cpu_sum_pheromones_initial << "\n";
    std::cout << "Initial sum of GPU pheromones: " << gpu_sum_pheromones_initial << "\n";
    std::cout << "Final sum of CPU pheromones:   " << cpu_sum_pheromones_final << "\n";
    std::cout << "Final sum of GPU pheromones:   " << gpu_sum_pheromones_final << "\n";

    EXPECT_GT(gpu_last_iters_average, cpu_last_iters_average * 0.99);
    EXPECT_GT(cpu_last_iters_average, gpu_last_iters_average * 0.99);
    EXPECT_GT(cpu_sum_pheromones_final, cpu_sum_pheromones_initial * 3);
    EXPECT_GT(gpu_sum_pheromones_final, gpu_sum_pheromones_initial * 3);
}