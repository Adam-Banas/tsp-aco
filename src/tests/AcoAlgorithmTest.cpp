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
