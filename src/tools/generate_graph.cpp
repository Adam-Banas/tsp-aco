#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include "../AcoGraph.hpp"

using Path = std::vector<std::size_t>; // Indices of cities in order

auto path_length(const aco::Graph& graph, const Path& path) {
    int length = 0;
    for (int i = 0; i < path.size(); ++i) {
        // Path stores visited cities in order. It is a round trip, so the last distance is from the
        // last city directly to the first one
        auto src = path[i];
        auto dst = path[(i + 1) % path.size()];

        length += graph.get_cost(src, dst);
    }

    return length;
}

const Path& get_shortest_path(const aco::Graph& graph, const std::vector<Path>& paths) {
    if (paths.empty()) {
        throw std::runtime_error("get_shortest_path: Empty paths container!");
    }

    return *std::min_element(begin(paths), end(paths), [&](const Path& a, const Path& b) {
        return path_length(graph, a) < path_length(graph, b);
    });
}

std::ostream& operator<<(std::ostream& out, const std::vector<aco::Graph::Index>& path) {
    out << "[";
    for (auto elem : path) {
        out << elem << " ";
    }
    out << "]";
    return out;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    if (argc != 3) {
        std::cout << "A tool to generate graph.\n";
        std::cout << "Usage: " << argv[0] << " cities filename\n";
        return 1;
    }
    std::size_t cities = std::stoi(argv[1]);
    std::string filename(argv[2]);

    // Initialization
    std::random_device rd;
    std::mt19937       gen(rd());
    aco::Graph         graph = aco::Graph(gen, cities, /*min_pheromone=*/0.1);

    // Write to file
    std::ofstream file(filename);
    file << graph.to_string();

    // Report success
    std::cout << "Successfully generated graph with " << cities
              << " cities and saved to: " << filename << "\n";
}