#include "AcoAlgorithmGpu.hpp"

#include <iostream>

#include "Utils.hpp"

namespace aco {

// TODO: Kernels should go to a separate file and be tested.

// TODO: Elementwise kernels (calculate_edge_scores, evaporate) don't need to be two-dimensional.
// Verify if it would be faster to just make them one-dimensional.

// Kernel that calculates scores for travelling from city to city
__global__ void kernel_calculate_edge_scores(int* costs, float* pheromones, float* out_scores,
                                             std::size_t nodes) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x < nodes && y < nodes && x != y) {
        auto i = x * nodes + y;
        out_scores[i] = pheromones[i] / costs[i];
    }
}

// Updates pheromones on all edges due to evaporation
__global__ void kernel_evaporate(float* pheromones, float evaporation_coefficient,
                                 float min_pheromone, std::size_t nodes) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x < nodes && y < nodes && x != y) {
        auto i = x * nodes + y;
        auto updated = pheromones[i] * evaporation_coefficient;
        pheromones[i] = updated > min_pheromone ? updated : min_pheromone;
    }
}

// TODO: This could be further parallelized.
// At the moment, each thread calculates pheromones for one agent path. It consists of two parts:
// - calculating the total distance,
// - using this distance to update pheromone on every edge on that path.
// Both above parts can be done in parallel by cities_count threads (the first part may be tricky
// because of synchronization).
__global__ void kernel_add_ants_pheromones(int* costs, float* pheromones, std::size_t* paths,
                                           std::size_t total_threads, std::size_t path_size) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < total_threads) {
        // The path for the current thread
        std::size_t* path = paths + i * path_size;

        // Step 1: Calculate total pheromone left by this agent
        int total_distance = 0;
        for (std::size_t j = 0; j < path_size; ++j) {
            // Path stores visited cities in order. It is a round trip, so the last distance is
            // from the last city directly to the first one.
            auto src = path[j];
            auto dst = path[(j + 1) % path_size];

            total_distance += costs[src * path_size + dst];
        }
        float total_pheromone = 1.f / total_distance;

        // Step 2: Update all visited edges
        for (std::size_t j = 0; j < path_size; ++j) {
            // Path stores visited cities in order. It is a round trip, so the last distance is
            // from the last city directly to the first one.
            auto src = path[j];
            auto dst = path[(j + 1) % path_size];

            // Calculate pheromone to leave on this section and update two-ways
            // Atomic add is needed, because other threads can update the same edges
            int   index_1 = src * path_size + dst;
            int   index_2 = dst * path_size + src;
            float pheromone_to_leave = total_pheromone / costs[index_1];
            atomicAdd(pheromones + index_1, pheromone_to_leave);
            atomicAdd(pheromones + index_2, pheromone_to_leave);
        }
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

// Assumes vector of vectors of the same size
template <typename T> static void send_to_device(T* dst, const std::vector<std::vector<T>>& src) {
    if (src.empty()) {
        // Nothing to send
        return;
    }

    auto           chunks = src.size();
    auto           chunk_size = src[0].size();
    auto           elements_count = chunks * chunk_size;
    std::vector<T> intermediate(elements_count);
    for (std::size_t i = 0; i < chunks; ++i) {
        for (std::size_t j = 0; j < chunk_size; ++j) {
            intermediate[i * chunk_size + j] = src[i][j];
        }
    }
    auto buffer_size_in_bytes = intermediate.size() * sizeof(T);
    auto res = cudaMemcpy(dst, intermediate.data(), buffer_size_in_bytes, cudaMemcpyHostToDevice);

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

AlgorithmGpu::AlgorithmGpu(std::mt19937& random_generator, Graph graph_arg, Config config_arg)
    : Algorithm(random_generator, std::move(graph_arg), config_arg), shortest_path(),
      costs(nullptr), pheromones(nullptr), scores(nullptr), paths(nullptr) {
    shortest_path = make_valid_path(graph);

    // Initialize CUDA, allocate buffers
    initialize_cuda();
    auto nodes = graph.get_size();
    auto edges = nodes * nodes;
    costs = allocate_on_device<int>(edges);
    pheromones = allocate_on_device<float>(edges);
    scores = allocate_on_device<float>(edges);
    paths = allocate_on_device<std::size_t>(config.agents_count * nodes);

    // Send costs from host graph to device (these never change, so it can be done just once)
    send_to_device(costs, graph.costs);
}

AlgorithmGpu::~AlgorithmGpu() {
    // TODO: Some abstraction for buffers would be useful
    free_device_buffer(costs);
    free_device_buffer(pheromones);
    free_device_buffer(scores);
}

const Graph& AlgorithmGpu::get_graph() const {
    // This is not a problem at the moment, but will be soon. Commit that stores pheromones directly
    // on device is coming.
    std::cerr << "[WARN] No synchronization AlgorithmGpu::get_graph\n";
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
    update_pheromones(paths);

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

// Calculate on GPU. Works probably much slower than CPU, because of all these allocations and data
// transfers.
std::vector<float> AlgorithmGpu::calculate_path_scores() const {
    auto cities = graph.get_size();
    auto buffer_size = cities * cities;

    // Send data to device
    // TODO: In general, pheromones can be stored and updated directly on device
    send_to_device(pheromones, graph.pheromones);

    // Launch kernel
    auto threads_per_block = 16;
    dim3 block_size(threads_per_block, threads_per_block);
    auto blocks_per_grid_dim =
        (buffer_size + threads_per_block - 1) / threads_per_block; // Rounded up
    dim3 blocks_per_grid(blocks_per_grid_dim, blocks_per_grid_dim);
    kernel_calculate_edge_scores<<<blocks_per_grid, block_size>>>(costs, pheromones, scores,
                                                                  cities);

    auto res = cudaGetLastError();
    if (res != cudaSuccess) {
        std::cerr << "Failed to launch scores calculation kernel! Error code: "
                  << cudaGetErrorString(res) << "\n";
        throw std::runtime_error("Failed to launch scores calculation kernel!");
    }

    // Get data
    std::vector<float> scores_host(buffer_size);
    send_to_host(scores_host, scores);

    return scores_host;
}

void AlgorithmGpu::update_pheromones(const std::vector<Path>& paths) {
    // Step 1: evaporation
    evaporate();

    // Step 2: Pheromones left by ants.
    add_ants_pheromones(paths);
}

void AlgorithmGpu::evaporate() {
    auto cities = graph.get_size();
    auto buffer_size = cities * cities;

    // Send data to device
    // TODO: In general, pheromones can be stored and updated directly on device
    send_to_device(pheromones, graph.pheromones);

    // Launch kernel
    auto threads_per_block = 16;
    dim3 block_size(threads_per_block, threads_per_block);
    auto blocks_per_grid_dim =
        (buffer_size + threads_per_block - 1) / threads_per_block; // Rounded up
    dim3 blocks_per_grid(blocks_per_grid_dim, blocks_per_grid_dim);
    kernel_evaporate<<<blocks_per_grid, block_size>>>(pheromones, config.pheromone_evaporation,
                                                      graph.initial_pheromone, cities);

    auto res = cudaGetLastError();
    if (res != cudaSuccess) {
        std::cerr << "Failed to launch evaporation kernel! Error code: " << cudaGetErrorString(res)
                  << "\n";
        throw std::runtime_error("Failed to launch evaporation kernel!");
    }

    // Send to host
    send_to_host(graph.pheromones, pheromones);
}

void AlgorithmGpu::add_ants_pheromones(const std::vector<Path>& travelled_paths) {
    auto cities = graph.get_size();
    auto total_threads = config.agents_count;

    // Send data to device
    // TODO: In general, pheromones can be stored and updated directly on device
    send_to_device(pheromones, graph.pheromones);
    send_to_device(paths, travelled_paths);

    // Launch kernel
    int  block_size = 256;
    auto blocks_per_grid = (total_threads + block_size - 1) / block_size; // Rounded up
    kernel_add_ants_pheromones<<<blocks_per_grid, block_size>>>(costs, pheromones, paths,
                                                                total_threads, cities);

    auto res = cudaGetLastError();
    if (res != cudaSuccess) {
        std::cerr << "Failed to launch add_ants_pheromones kernel! Error code: "
                  << cudaGetErrorString(res) << "\n";
        throw std::runtime_error("Failed to launch add_ants_pheromones kernel!");
    }

    // Send pheromones back to host
    send_to_host(graph.pheromones, pheromones);
}

} // namespace aco