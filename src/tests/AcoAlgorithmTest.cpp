#include <gtest/gtest.h>
#include <utility>
#include <vector>

#include "../AcoAlgorithmCpu.hpp"
#include "../AcoGraph.hpp"

using aco::Algorithm;
using aco::AlgorithmCpu;
using aco::Graph;

class AcoAlgorithmTest : public ::testing::Test {
  public:
    AcoAlgorithmTest() : gen(/*seed=*/42) {}

  public:
    // Convenience function, to avoid passing gen every time
    AlgorithmCpu make_algorithm(aco::Graph graph, Algorithm::Config config) {
        return AlgorithmCpu(gen, std::move(graph), config);
    }

    // Path should be of proper length, every index should be valid and unique
    void validate_path(const Graph& graph, Algorithm::Path path) {
        // Verify size
        EXPECT_EQ(graph.get_size(), path.size()) << "Path size incorrect.";

        // Verify that indices are valid
        EXPECT_TRUE(std::all_of(begin(path), end(path), [graph_size = graph.get_size()](auto elem) {
            return elem < graph_size;
        })) << "Path elements are are not valid graph indices (out of range).";

        // Verify that indices are unique
        std::sort(begin(path), end(path));
        EXPECT_EQ(end(path), std::adjacent_find(begin(path), end(path)))
            << "Path elements are not unique.";
    }

  public:
    std::mt19937 gen;
};

TEST_F(AcoAlgorithmTest, ThrowsOnInvalidArguments) {
    std::size_t nodes = 30;
    Graph       graph(gen, nodes, /*initial_pheromone=*/0.01);

    const Algorithm::Config correct_config{/*agents_count=*/nodes, /*pheromone_evaporation=*/0.9};

    {
        // Agents count zero
        auto config = correct_config;
        config.agents_count = 0;
        EXPECT_THROW(make_algorithm(graph, config), std::invalid_argument)
            << "Should throw on agents count equal to zero.";
    }

    {
        // Negative pheromone evaporation
        auto config = correct_config;
        config.pheromone_evaporation = -1;
        EXPECT_THROW(make_algorithm(graph, config), std::invalid_argument)
            << "Should throw on negative pheromone evaporation coefficient.";
    }

    {
        // Pheromone evaporation > 1
        auto config = correct_config;
        config.pheromone_evaporation = 1.1;
        EXPECT_THROW(make_algorithm(graph, config), std::invalid_argument)
            << "Should throw on pheromone evaporation coefficient greater than one.";
    }
}

TEST_F(AcoAlgorithmTest, GetGraph) {
    std::size_t nodes = 30;
    Graph       graph(gen, nodes, /*initial_pheromone=*/0.01);

    const Algorithm::Config config{/*agents_count=*/nodes, /*pheromone_evaporation=*/0.9};

    auto algorithm = make_algorithm(graph, config);

    EXPECT_EQ(graph, algorithm.get_graph());
}

TEST_F(AcoAlgorithmTest, InitialPathIsValid) {
    std::size_t nodes = 30;
    Graph       graph(gen, nodes, /*initial_pheromone=*/0.01);

    const Algorithm::Config config{/*agents_count=*/nodes, /*pheromone_evaporation=*/0.9};
    auto                    algorithm = make_algorithm(graph, config);

    auto path = algorithm.get_shortest_path();
    validate_path(graph, path);
}

TEST_F(AcoAlgorithmTest, PheromonesAreUpdatedAfterSimulation) {
    // Initialize
    std::size_t nodes = 30;
    Graph       initial_graph(gen, nodes, /*initial_pheromone=*/0.01);

    const Algorithm::Config config{/*agents_count=*/nodes, /*pheromone_evaporation=*/0.9};
    auto                    algorithm = make_algorithm(initial_graph, config);

    // Run simulation
    algorithm.advance();

    // Compare with initial graph
    EXPECT_NE(initial_graph, algorithm.get_graph());
}

TEST_F(AcoAlgorithmTest, ShortestPathIsValidAfterEveryIteration) {
    // Initialize
    std::size_t nodes = 30;
    Graph       graph(gen, nodes, /*initial_pheromone=*/0.01);

    const Algorithm::Config config{/*agents_count=*/nodes, /*pheromone_evaporation=*/0.9};
    auto                    algorithm = make_algorithm(graph, config);

    // Run simulation
    for (int i = 0; i < 20; ++i) {
        algorithm.advance();

        // Verify after every step
        auto path = algorithm.get_shortest_path();
        validate_path(graph, path);
    }
}

TEST_F(AcoAlgorithmTest, FinalPathDifferFromTheInitialOne) {
    std::size_t nodes = 30;
    Graph       graph(gen, nodes, /*initial_pheromone=*/0.01);

    const Algorithm::Config config{/*agents_count=*/nodes, /*pheromone_evaporation=*/0.9};
    auto                    algorithm = make_algorithm(graph, config);

    auto initial_path = algorithm.get_shortest_path();

    // Run simulation
    for (int i = 0; i < 20; ++i) {
        algorithm.advance();
    }

    // Compare
    auto final_path = algorithm.get_shortest_path();
    EXPECT_NE(initial_path, final_path);
}

TEST_F(AcoAlgorithmTest, IterationBestPathIsValidAfterEveryIteration) {
    // Initialize
    std::size_t nodes = 30;
    Graph       graph(gen, nodes, /*initial_pheromone=*/0.01);

    const Algorithm::Config config{/*agents_count=*/nodes, /*pheromone_evaporation=*/0.9};
    auto                    algorithm = make_algorithm(graph, config);

    // Run simulation
    for (int i = 0; i < 20; ++i) {
        // Verify after every step
        Algorithm::Path iter_best = algorithm.advance();
        validate_path(graph, iter_best);
    }
}

TEST_F(AcoAlgorithmTest, CompareIterationBestPathAndBestSoFar) {
    // Initialize
    std::size_t nodes = 30;
    Graph       graph(gen, nodes, /*initial_pheromone=*/0.01);

    const Algorithm::Config config{/*agents_count=*/nodes, /*pheromone_evaporation=*/0.9};
    auto                    algorithm = make_algorithm(graph, config);

    const int max_iterations = 20;
    int       equal_count = 0;

    // Run simulation
    for (int i = 0; i < max_iterations; ++i) {
        Algorithm::Path iter_best = algorithm.advance();
        Algorithm::Path best_so_far = algorithm.get_shortest_path();

        if (iter_best == best_so_far) {
            ++equal_count;
        }
    }

    // Expect that in some iterations best path is the best total, some don't
    EXPECT_GT(equal_count, 0);
    EXPECT_LT(equal_count, max_iterations);
}