#include "AcoGraph.hpp"

namespace aco {

Graph::Graph(int nodes, float initial_pheromone) {}

int Graph::get_cost(Index src, Index dst) const {
    return 0;
}

float Graph::get_pheromone(Index src, Index dst) const {
    return 0;
}

float Graph::set_pheromone(Index src, Index dst, float value) {
    return 0;
}

void Graph::add_pheromone_two_way(Index a, Index b, float amount) {}

void Graph::update_all(float coefficient) {}

} // namespace aco