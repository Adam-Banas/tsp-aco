#include "AcoGraph.hpp"

#include <iostream>
#include <stdexcept>

namespace aco {

Graph::Graph(std::mt19937& random_generator, std::size_t nodes_input, float initial_pheromone)
    : costs(nodes_input * nodes_input), pheromones(nodes_input * nodes_input), nodes(nodes_input) {

    std::uniform_int_distribution<> distrib(1, /*max_dist=*/10);

    // Populate distances graph
    for (int i = 0; i < nodes; ++i) {
        for (int j = i + 1; j < nodes; ++j) {
            auto dist = distrib(random_generator);
            costs.at(internal_index(i, j)) = dist;
            costs.at(internal_index(j, i)) = dist;
        }
    }
}

int Graph::get_cost(Index src, Index dst) const {
    return costs.at(internal_index(src, dst));
}

float Graph::get_pheromone(Index src, Index dst) const {
    return 0;
}

float Graph::set_pheromone(Index src, Index dst, float value) {
    return 0;
}

void Graph::add_pheromone_two_way(Index a, Index b, float amount) {}

void Graph::update_all(float coefficient) {}

Graph::Index Graph::internal_index(Index src, Index dst) const {
    if (src == dst || src >= nodes || dst >= nodes) {
        std::cerr << "aco::Graph invalid arguments. Graph size: " << nodes << ", src : " << src
                  << ", dst: " << dst << std::endl;
        throw std::invalid_argument("AcoGraph invalid src and / or dst arguments!");
    }

    return src * nodes + dst;
}

} // namespace aco