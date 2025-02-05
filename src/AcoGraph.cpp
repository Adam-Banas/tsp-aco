#include "AcoGraph.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "../third_party/nlohmann/json.hpp"

namespace aco {

Graph::Graph(std::mt19937& random_generator, std::size_t nodes_input, float init_pheromone)
    : costs(nodes_input * nodes_input), pheromones(nodes_input * nodes_input, init_pheromone),
      nodes(nodes_input), initial_pheromone(init_pheromone) {

    std::uniform_int_distribution<> distrib(1, /*max_dist=*/nodes_input);

    // Populate distances graph
    for (int i = 0; i < nodes; ++i) {
        for (int j = i + 1; j < nodes; ++j) {
            auto dist = distrib(random_generator);
            costs.at(internal_index(i, j)) = dist;
            costs.at(internal_index(j, i)) = dist;
        }
    }
}

Graph::Graph(std::vector<int> costs_arg, std::vector<float> pheromones_arg, std::size_t nodes,
             float initial_pheromone)
    : costs(std::move(costs_arg)), pheromones(std::move(pheromones_arg)), nodes(nodes),
      initial_pheromone(initial_pheromone) {
    // Verify the sizes of containers
    const auto expected_size = nodes * nodes;
    if (expected_size != costs.size()) {
        std::cerr << "Graph constructor: Incorrect costs vector size! Expected: " << expected_size
                  << ", got: " << costs.size() << "\n";
        throw std::invalid_argument("Graph constructor: Incorrect costs vector size!");
    }

    if (expected_size != pheromones.size()) {
        std::cerr << "Graph constructor: Incorrect pheromones vector size! Expected: "
                  << expected_size << ", got: " << pheromones.size() << "\n";
        throw std::invalid_argument("Graph constructor: Incorrect pheromones vector size!");
    }
}

std::size_t Graph::get_size() const {
    return nodes;
}

int Graph::get_cost(Index src, Index dst) const {
    return costs.at(internal_index(src, dst));
}

float Graph::get_pheromone(Index src, Index dst) const {
    return pheromones.at(internal_index(src, dst));
}

void Graph::set_pheromone(Index src, Index dst, float value) {
    pheromones.at(internal_index(src, dst)) = std::max(value, initial_pheromone);
}

void Graph::add_pheromone_two_way(Index a, Index b, float amount) {
    // a -> b
    auto index = internal_index(a, b);
    pheromones.at(index) += amount;

    // b -> a
    index = internal_index(b, a);
    pheromones.at(index) += amount;
}

void Graph::update_all(float coefficient) {
    std::transform(begin(pheromones), end(pheromones), begin(pheromones),
                   [&](auto elem) { return std::max(elem * coefficient, initial_pheromone); });
}

std::string Graph::to_string() const {
    auto json = nlohmann::json{{"costs", costs},
                               {"pheromones", pheromones},
                               {"nodes", nodes},
                               {"initial_pheromone", initial_pheromone}};
    return json.dump(/*indent=*/4);
}

Graph Graph::from_string(const std::string& string) {
    try {
        nlohmann::json json = nlohmann::json::parse(string);
        auto           costs = json.at("costs").get<std::vector<int>>();
        auto           pheromones = json.at("pheromones").get<std::vector<float>>();
        auto           nodes = json.at("nodes").get<std::size_t>();
        auto           initial_pheromone = json.at("initial_pheromone").get<float>();
        return Graph(std::move(costs), std::move(pheromones), nodes, initial_pheromone);
    } catch (const nlohmann::detail::exception& e) {
        // Hide implementation details (exceptions from json library), throw a generic one.
        std::cerr << "Exception thrown in graph deserialization! What: " << e.what() << "\n";
        throw std::invalid_argument("Exception thrown in graph deserialization!");
    }
}

Graph::Index Graph::internal_index(Index src, Index dst) const {
    if (src == dst || src >= nodes || dst >= nodes) {
        std::cerr << "aco::Graph invalid arguments. Graph size: " << nodes << ", src : " << src
                  << ", dst: " << dst << std::endl;
        throw std::invalid_argument("AcoGraph invalid src and / or dst arguments!");
    }

    return src * nodes + dst;
}

bool operator==(const Graph& lhs, const Graph& rhs) {
    return lhs.costs == rhs.costs && lhs.pheromones == rhs.pheromones && lhs.nodes == rhs.nodes &&
           lhs.initial_pheromone == rhs.initial_pheromone;
}

} // namespace aco