#include <gtest/gtest.h>
#include <utility>
#include <vector>

#include "../AcoAlgorithm.hpp"
#include "../AcoGraph.hpp"

using aco::Algorithm;
using aco::Graph;

class AcoAlgorithmTest : public ::testing::Test {
  public:
    AcoAlgorithmTest() : gen(/*seed=*/42) {}

  public:
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
    std::size_t nodes = 10;
    Graph       graph(gen, nodes, /*initial_pheromone=*/0.01);

    const Algorithm::Config correct_config{/*agents_count=*/nodes, /*pheromone_evaporation=*/0.9};

    {
        // Agents count zero
        auto config = correct_config;
        config.agents_count = 0;
        EXPECT_THROW(Algorithm(graph, config), std::invalid_argument)
            << "Should throw on agents count equal to zero.";
    }

    {
        // Negative pheromone evaporation
        auto config = correct_config;
        config.pheromone_evaporation = -1;
        EXPECT_THROW(Algorithm(graph, config), std::invalid_argument)
            << "Should throw on negative pheromone evaporation coefficient.";
    }

    {
        // Pheromone evaporation > 1
        auto config = correct_config;
        config.pheromone_evaporation = 1.1;
        EXPECT_THROW(Algorithm(graph, config), std::invalid_argument)
            << "Should throw on pheromone evaporation coefficient greater than one.";
    }
}

TEST_F(AcoAlgorithmTest, GetGraph) {
    std::size_t nodes = 10;
    Graph       graph(gen, nodes, /*initial_pheromone=*/0.01);

    const Algorithm::Config config{/*agents_count=*/nodes, /*pheromone_evaporation=*/0.9};

    Algorithm algorithm(graph, config);

    EXPECT_EQ(graph, algorithm.get_graph());
}

TEST_F(AcoAlgorithmTest, InitialPathIsValid) {
    std::size_t nodes = 10;
    Graph       graph(gen, nodes, /*initial_pheromone=*/0.01);

    const Algorithm::Config config{/*agents_count=*/nodes, /*pheromone_evaporation=*/0.9};
    Algorithm               algorithm(graph, config);

    auto path = algorithm.get_shortest_path();
    validate_path(graph, path);
}
