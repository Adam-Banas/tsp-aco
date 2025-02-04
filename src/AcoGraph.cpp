#include "AcoGraph.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

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

Graph::Index Graph::internal_index(Index src, Index dst) const {
    if (src == dst || src >= nodes || dst >= nodes) {
        std::cerr << "aco::Graph invalid arguments. Graph size: " << nodes << ", src : " << src
                  << ", dst: " << dst << std::endl;
        throw std::invalid_argument("AcoGraph invalid src and / or dst arguments!");
    }

    return src * nodes + dst;
}

bool operator==(const Graph& lhs, const Graph& rhs) {
    return lhs.costs == rhs.costs && lhs.pheromones == rhs.pheromones;
}

} // namespace aco