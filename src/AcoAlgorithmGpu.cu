#include "AcoAlgorithmGpu.hpp"

#include <iostream>

#include "Utils.hpp"

namespace aco {

// Kernel that calculates scores for travelling from city to city
__global__ void calculate_edge_scores(int* costs, float* pheromones, float* out_scores,
                                      std::size_t nodes) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x < nodes && y < nodes && x != y) {
        auto i = x * nodes + y;
        out_scores[i] = pheromones[i] / costs[i];
    }
}

// Initialize shortest path just to be valid
static auto make_valid_path(const Graph& graph) {
    Algorithm::Path result(graph.get_size());
    std::iota(begin(result), end(result), 0);
    return result;
}

static void initialize_cuda() {
    auto res = cudaSetDevice(0);

    if (res != cudaSuccess) {
        std::cerr << "Failed to initialize CUDA! Error code: " << cudaGetErrorString(res) << "\n";
        throw std::runtime_error("Failed to initialize CUDA");
    }
}

AlgorithmGpu::AlgorithmGpu(std::mt19937& random_generator, Graph graph_arg, Config config_arg)
    : Algorithm(random_generator, std::move(graph_arg), config_arg), shortest_path() {
    initialize_cuda();

    shortest_path = make_valid_path(graph);
}

const Graph& AlgorithmGpu::get_graph() const {
    return graph;
}

const AlgorithmGpu::Path& AlgorithmGpu::get_shortest_path() const {
    return shortest_path;
}

AlgorithmGpu::Path AlgorithmGpu::advance() {
    auto cities = graph.get_size();
    Path iteration_best = make_valid_path(graph);

    // Calculate path scores on GPU.
    // It works slower than CPU counterpart, because there's a lot of data movement.
    auto path_scores = calculate_path_scores();

    // Generate solutions
    std::vector<Path> paths(config.agents_count);
    for (std::size_t i = 0; i < config.agents_count; ++i) {
        auto& path = paths[i];

        // Start from a city with index 'i', modulo in case the number of agents is higher than the
        // number of cities
        path.push_back(i % cities);

        // Choose one new destination in every iteration
        while (path.size() < cities) {
            // Calculate the score (desire to go) for every city
            auto               current_city = path.back();
            std::vector<float> scores(cities);
            for (std::size_t j = 0; j < cities; ++j) {
                if (utils::contains(path, j)) {
                    // Path already visited - leave it a score of zero
                    continue;
                }

                scores[j] = path_scores[current_city * cities + j];
            }

            // Choose the target city using roullette random algorithm
            auto target = utils::roullette(scores, gen);
            path.push_back(target);
        }

        // Path calculated - remember it if is shorter than the current best
        if (path_length(path) < path_length(iteration_best)) {
            iteration_best = path;
        }
    }

    // Update pheromones
    // Step 1: evaporation
    graph.update_all(config.pheromone_evaporation);

    // Step 2: Pheromones left by ants.
    // Basic algorithm, where every ant leaves pheromones, and the amount is independent from
    // other ants' solutions.
    // No limit on total pheromone on a section.
    for (const auto& path : paths) {
        // The total amount of pheromone left by ant is inversely proportional to the distance
        // covered by ant.
        float total_pheromone = 1.f / path_length(path);

        for (int i = 0; i < path.size(); ++i) {
            // Path stores visited cities in order. It is a round trip, so the last distance is
            // from the last city directly to the first one
            auto src = path[i];
            auto dst = path[(i + 1) % path.size()];

            // The amount of pheromone to leave is proportional to the section length
            float pheromone_to_leave = total_pheromone / graph.get_cost(src, dst);
            graph.add_pheromone_two_way(src, dst, pheromone_to_leave);
        }
    }

    // If the iteration best path is shortest than the global shortest (best so far), remember it
    if (path_length(iteration_best) < path_length(shortest_path)) {
        shortest_path = iteration_best;
    }

    return iteration_best;
}

int AlgorithmGpu::path_length(const Path& path) const {
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

// Allocate a buffer of given type on the device
template <typename T> static T* allocate_on_device(std::size_t num_elements) {
    auto size_in_bytes = num_elements * sizeof(T);
    T*   buffer = nullptr;
    auto res = cudaMalloc((void**)&buffer, size_in_bytes);

    if (res != cudaSuccess) {
        std::cerr << "Failed to allocate device buffer! Error code: " << cudaGetErrorString(res)
                  << "\n";
        throw std::runtime_error("Failed to allocate device buffer");
    }

    return buffer;
}

template <typename T> void free_device_buffer(T* buffer) {
    auto res = cudaFree(buffer);
    if (res != cudaSuccess) {
        std::cerr << "Failed to free device buffer! Error code: " << cudaGetErrorString(res)
                  << "\n";
        throw std::runtime_error("Failed to free device buffer");
    }
}

// Send buffer from host to device
template <typename T> static void send_to_device(T* dst, const std::vector<T>& src) {
    auto size_in_bytes = src.size() * sizeof(T);
    auto res = cudaMemcpy(dst, src.data(), size_in_bytes, cudaMemcpyHostToDevice);

    if (res != cudaSuccess) {
        std::cerr << "Failed to send buffer to device! Error code: " << cudaGetErrorString(res)
                  << "\n";
        throw std::runtime_error("Failed to send buffer to device");
    }
}

// Send buffer from device to host
template <typename T> static void send_to_host(std::vector<T>& dst, T* src) {
    auto size_in_bytes = dst.size() * sizeof(T);
    auto res = cudaMemcpy(dst.data(), src, size_in_bytes, cudaMemcpyDeviceToHost);

    if (res != cudaSuccess) {
        std::cerr << "Failed to send buffer from device to host! Error code: "
                  << cudaGetErrorString(res) << "\n";
        throw std::runtime_error("Failed to send buffer from device to host");
    }
}

// Calculate on GPU. Works probably much slower than CPU, because of all these allocations and data
// transfers.
std::vector<float> AlgorithmGpu::calculate_path_scores() const {
    auto cities = graph.get_size();
    auto buffer_size = cities * cities;

    // Allocate buffers on device
    // TODO: This can be done once, at initilization stage
    int*   costs = allocate_on_device<int>(buffer_size);
    float* pheromones = allocate_on_device<float>(buffer_size);
    float* scores = allocate_on_device<float>(buffer_size);

    // Send data to device
    // TODO: costs do not change, they can be sent once, at initialization stage
    send_to_device(costs, graph.costs);
    // TODO: In general, pheromones can be stored and updated directly on device
    send_to_device(pheromones, graph.pheromones);

    // Launch kernel
    auto threads_per_block = 1;
    dim3 block_size(1, 1);
    auto blocks_per_grid_dim = (buffer_size + threads_per_block + 1) / threads_per_block; // Rounded up
    dim3 blocks_per_grid(blocks_per_grid_dim, blocks_per_grid_dim);
    calculate_edge_scores<<<blocks_per_grid, block_size>>>(costs, pheromones, scores,
                                                                  cities);

    auto res = cudaGetLastError();
    if (res != cudaSuccess) {
        std::cerr << "Failed to launch multiplication kernel! Error code: "
                  << cudaGetErrorString(res) << "\n";
        throw std::runtime_error("Failed to launch multiplication kernel");
    }

    // Get data
    std::vector<float> scores_host(buffer_size);
    send_to_host(scores_host, scores);

    // Free device buffers
    // TODO: Use RAII.
    free_device_buffer(costs);
    free_device_buffer(pheromones);
    free_device_buffer(scores);

    return scores_host;
}

} // namespace aco