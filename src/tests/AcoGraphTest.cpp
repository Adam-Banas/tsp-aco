#include <gtest/gtest.h>
#include <utility>
#include <vector>

#include "../AcoGraph.hpp"

using aco::Graph;

class AcoGraphTest : public ::testing::Test {
  public:
    AcoGraphTest() : gen(/*seed=*/42) {}

  public:
    std::mt19937 gen;
};

TEST_F(AcoGraphTest, ThrowsOnInvalidArguments) {
    std::size_t nodes = 10;
    Graph       graph(gen, nodes, /*initial_pheromone=*/1);

    std::vector<std::pair<Graph::Index, Graph::Index>> invalid_args{
        {nodes, 0}, // src out of bounds
        {0, nodes}, // dst out of bounds
        {0, 0}      // src and dst equal
    };

    for (auto elem : invalid_args) {
        EXPECT_THROW(graph.get_cost(elem.first, elem.second), std::invalid_argument);
        EXPECT_THROW(graph.get_pheromone(elem.first, elem.second), std::invalid_argument);
    }
}

TEST_F(AcoGraphTest, CostsAreNonZeroInitialized) {
    std::size_t nodes = 10;
    Graph       graph(gen, nodes, /*initial_pheromone=*/1);

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

TEST_F(AcoGraphTest, CostsAreInitializedSymmetrically) {
    std::size_t nodes = 10;
    Graph       graph(gen, nodes, /*initial_pheromone=*/1);

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

TEST_F(AcoGraphTest, PheromonesAreInitialized) {
    std::size_t nodes = 10;
    float       initial_pheromone = 0.7;
    Graph       graph(gen, nodes, initial_pheromone);

    for (int i = 0; i < nodes; ++i) {
        for (int j = 0; j < nodes; ++j) {
            if (i == j) {
                continue;
            }

            // Every path should have pheromone initialized
            EXPECT_EQ(initial_pheromone, graph.get_pheromone(i, j));
        }
    }
}