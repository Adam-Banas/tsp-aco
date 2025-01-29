#include <algorithm>
#include <vector>

#ifndef UTILS_HPP
#define UTILS_HPP

namespace utils {

template <typename T> bool contains(const std::vector<T>& c, T elem) {
    return std::find(begin(c), end(c), elem) != end(c);
}

std::size_t roullette(const std::vector<float>& scores, std::mt19937& gen) {
    auto sum = std::accumulate(begin(scores), end(scores), .0);

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

#endif // UTILS_HPP