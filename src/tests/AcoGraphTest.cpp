#include <gtest/gtest.h>
#include <utility>
#include <vector>

#include "../../third_party/nlohmann/json.hpp"
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
        EXPECT_THROW(graph.set_pheromone(elem.first, elem.second, /*value=*/0.7),
                     std::invalid_argument);
        EXPECT_THROW(graph.add_pheromone_two_way(elem.first, elem.second, /*value=*/0.7),
                     std::invalid_argument);
    }
}

TEST_F(AcoGraphTest, GetSize) {
    std::size_t nodes = 10;
    Graph       graph(gen, nodes, /*initial_pheromone=*/1);

    EXPECT_EQ(nodes, graph.get_size());
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

TEST_F(AcoGraphTest, ManuallyCompareAfterCopy) {
    std::size_t nodes = 30;
    float       initial_pheromone = 0.7;
    Graph       graph(gen, nodes, initial_pheromone);

    Graph copy = graph;

    for (int i = 0; i < nodes; ++i) {
        for (int j = 0; j < nodes; ++j) {
            if (i == j) {
                continue;
            }

            EXPECT_EQ(graph.get_cost(i, j), copy.get_cost(i, j));
            EXPECT_EQ(graph.get_pheromone(i, j), copy.get_pheromone(i, j));
        }
    }
}

TEST_F(AcoGraphTest, ComparisonOperatorAfterCopy) {
    std::size_t nodes = 30;
    float       initial_pheromone = 0.7;
    Graph       graph(gen, nodes, initial_pheromone);

    Graph copy = graph;

    EXPECT_EQ(graph, copy);
}

TEST_F(AcoGraphTest, ComparisonOperatorOnGraphsWithDifferentCosts) {
    std::size_t nodes = 30;
    float       initial_pheromone = 0.7;
    Graph       first(gen, nodes, initial_pheromone);
    Graph       second(gen, nodes, initial_pheromone);

    EXPECT_NE(first, second);
}

TEST_F(AcoGraphTest, ComparisonOperatorOnGraphsWithDifferentPheromones) {
    std::size_t nodes = 30;
    float       initial_pheromone = 0.7;
    Graph       first(gen, nodes, initial_pheromone);

    // Second graph as a copy of the first one, with changed amounts of pheromones
    Graph second = first;
    second.set_pheromone(/*src=*/3, /*dst*/ 4, /*value=*/1.7);

    EXPECT_NE(first, second);
}

TEST_F(AcoGraphTest, SetPheromone) {
    std::size_t nodes = 10;
    float       initial_pheromone = 0.7;
    Graph       graph(gen, nodes, initial_pheromone);

    float        new_pheromone = 0.9;
    Graph::Index src = 3, dst = 4;
    graph.set_pheromone(src, dst, new_pheromone);
    EXPECT_EQ(new_pheromone, graph.get_pheromone(src, dst));
}

TEST_F(AcoGraphTest, AddPheromoneTwoWay) {
    std::size_t nodes = 10;
    float       initial_pheromone = 0.7;
    Graph       graph(gen, nodes, initial_pheromone);

    float        added = 0.5;
    Graph::Index src = 3, dst = 4;
    graph.add_pheromone_two_way(src, dst, added);

    // Pheromone should be added both ways
    float new_pheromone = initial_pheromone + added;
    EXPECT_EQ(new_pheromone, graph.get_pheromone(src, dst));
    EXPECT_EQ(new_pheromone, graph.get_pheromone(dst, src));
}

TEST_F(AcoGraphTest, UpdateAll) {
    // Initialize
    std::size_t nodes = 10;
    float       initial_pheromone = 0.7;
    Graph       graph(gen, nodes, initial_pheromone);

    // Update
    float update_coefficient = 1.2;
    graph.update_all(update_coefficient);

    // All elements should be updated
    float new_pheromone = initial_pheromone * update_coefficient;
    for (Graph::Index i = 0; i < nodes; ++i) {
        for (Graph::Index j = 0; j < nodes; ++j) {
            if (i == j) {
                continue;
            }
            EXPECT_EQ(new_pheromone, graph.get_pheromone(i, j));
        }
    }
}

TEST_F(AcoGraphTest, UpdateAllShouldNotGoBelowInitial) {
    // Initialize
    std::size_t nodes = 10;
    float       initial_pheromone = 0.7;
    Graph       graph(gen, nodes, initial_pheromone);

    // Set one value to above initial level
    float        pheromone_after_increase = 1.9;
    Graph::Index src = 5, dst = 2;
    graph.set_pheromone(src, dst, pheromone_after_increase);

    // Initial pheromone is the lowest possible value, so the following shouldn't have any effect on
    // most elements
    float update_coefficient = 0.9;
    graph.update_all(update_coefficient);

    // Verify
    float pheromone_after_update = pheromone_after_increase * update_coefficient;
    for (Graph::Index i = 0; i < nodes; ++i) {
        for (Graph::Index j = 0; j < nodes; ++j) {
            if (i == j) {
                continue;
            }
            auto expected = (i == src && j == dst) ? pheromone_after_update : initial_pheromone;
            EXPECT_EQ(expected, graph.get_pheromone(i, j));
        }
    }
}

TEST_F(AcoGraphTest, SerializeDeserialize) {
    // Create graph, change some pheromone values
    std::size_t nodes = 10;
    float       initial_pheromone = 0.7;
    Graph       graph(gen, nodes, initial_pheromone);

    float        pheromone_after_increase = 1.9;
    Graph::Index src = 5, dst = 2;
    graph.set_pheromone(src, dst, pheromone_after_increase);

    // Serialize and deserialize
    auto serialized = graph.to_string();

    std::cout << serialized << "\n";
    Graph deserialized = Graph::from_string(serialized);

    // Compare
    EXPECT_EQ(graph, deserialized);
}

// The following deserialization failure tests are implementation-defined, which is a bad practice
// in general. However, because the serialized object is a human-readable JSON, it is exposed for
// manual modification, therefore deserialization needs to verify input JSON's validity. Can't test
// it without dependency on implementaion.
TEST_F(AcoGraphTest, DeserializeThrowsOnInvalidJson) {
    std::string invalid("Not a json");
    EXPECT_THROW(Graph::from_string(invalid), std::invalid_argument);
}

TEST_F(AcoGraphTest, DeserializeThrowsOnMissingJsonFields) {
    // Create graph, change some pheromone values
    std::size_t nodes = 10;
    float       initial_pheromone = 0.7;
    Graph       graph(gen, nodes, initial_pheromone);

    // Remove some fields
    auto json = nlohmann::json::parse(graph.to_string());
    json.erase("costs");

    // Expect it throws
    EXPECT_THROW(Graph::from_string(json.dump()), std::invalid_argument);
}

TEST_F(AcoGraphTest, DeserializeThrowsOnIncorrectVectorSizes) {
    // Create graph, change some pheromone values
    std::size_t nodes = 10;
    float       initial_pheromone = 0.7;
    Graph       graph(gen, nodes, initial_pheromone);

    // Remove an element from costs vector
    auto json = nlohmann::json::parse(graph.to_string());
    json.at("costs").erase(0);

    // Expect it throws
    EXPECT_THROW(Graph::from_string(json.dump()), std::invalid_argument);
}

TEST_F(AcoGraphTest, DeserializeThrowsOnIncorrectVectorSizes2) {
    // Create graph, change some pheromone values
    std::size_t nodes = 10;
    float       initial_pheromone = 0.7;
    Graph       graph(gen, nodes, initial_pheromone);

    // Remove an element from pheromones vector
    auto json = nlohmann::json::parse(graph.to_string());
    json.at("pheromones").erase(0);

    // Expect it throws
    EXPECT_THROW(Graph::from_string(json.dump()), std::invalid_argument);
}

TEST_F(AcoGraphTest, DeserializeThrowsOnIncorrectVectorSizes3) {
    // Create graph, change some pheromone values
    std::size_t nodes = 10;
    float       initial_pheromone = 0.7;
    Graph       graph(gen, nodes, initial_pheromone);

    // Change the value of "nodes" field
    auto json = nlohmann::json::parse(graph.to_string());
    json.at("nodes") = 3;

    // Expect it throws
    EXPECT_THROW(Graph::from_string(json.dump()), std::invalid_argument);
}