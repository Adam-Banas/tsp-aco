#include <gtest/gtest.h>

#include "../AcoGraph.hpp"

class AcoGraphTest : public ::testing::Test {
  public:
    AcoGraphTest() : gen(/*seed=*/42) {}

  public:
    std::mt19937 gen;
};

TEST_F(AcoGraphTest, CostIsNonZeroInitialized) {
    std::size_t nodes = 10;
    aco::Graph  graph(gen, nodes, /*initial_pheromone=*/1);

    for (int i = 0; i < nodes; ++i) {
        for (int j = 0; j < nodes; ++j) {
            if (i == j) {
                continue;
            }

            // Every path should have a positive cost
            EXPECT_GE(graph.get_cost(i, j), 1);
        }
    }
}

TEST_F(AcoGraphTest, CostIsInitializedSymmetrically) {
    std::size_t nodes = 10;
    aco::Graph  graph(gen, nodes, /*initial_pheromone=*/1);

    for (int i = 0; i < nodes; ++i) {
        for (int j = 0; j < nodes; ++j) {
            if (i == j) {
                continue;
            }

            // Graph should be symmetrical - cost to go from 'i' to 'j' should be the same as from
            // 'j' to 'i'
            EXPECT_EQ(graph.get_cost(i, j), graph.get_cost(j, i));
        }
    }
}

TEST_F(AcoGraphTest, ThrowsWhenIndexIsOutOfRange) {
    std::size_t nodes = 10;
    aco::Graph  graph(gen, nodes, /*initial_pheromone=*/1);

    EXPECT_THROW(graph.get_cost(nodes, 0), std::invalid_argument);
    EXPECT_THROW(graph.get_cost(0, nodes), std::invalid_argument);
}

TEST_F(AcoGraphTest, ThrowsWhenSrcAndDstAreTheSame) {
    std::size_t nodes = 10;
    aco::Graph  graph(gen, nodes, /*initial_pheromone=*/1);

    EXPECT_THROW(graph.get_cost(0, 0), std::invalid_argument);
    EXPECT_THROW(graph.get_cost(0, 0), std::invalid_argument);
}