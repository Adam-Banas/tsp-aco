#include "Utils.hpp"

#include <exception>
#include <iostream>
#include <numeric>

namespace utils {

std::size_t roullette(const std::vector<float>& scores, std::mt19937& gen) {
    auto sum = std::accumulate(begin(scores), end(scores), .0);

    if (sum == 0) {
        std::cerr << "Error in roullette algorithm. Sum: " << sum << "\n";
        throw std::runtime_error("Error in roullette algorithm!");
    }

    std::uniform_real_distribution<> distrib(0, sum);
    float                            random = distrib(gen);

    float partial = 0;
    for (std::size_t i = 0; i < scores.size(); ++i) {
        partial += scores[i];
        if (partial > random) {
            return i;
        }
    }

    if ((random - partial) < 1e-5) {
        return scores.size() - 1;
    }

    std::cerr << "Error in roullette algorithm. Sum: " << sum << ", random: " << random
              << ", partial: " << partial << "\n";
    throw std::runtime_error("Error in roullette algorithm");
}

} // namespace utils