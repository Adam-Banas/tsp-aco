#include <algorithm>
#include <random>
#include <vector>

#ifndef UTILS_HPP
#define UTILS_HPP

namespace utils {

template <typename T> bool contains(const std::vector<T>& c, T elem) {
    return std::find(begin(c), end(c), elem) != end(c);
}

std::size_t roullette(const std::vector<float>& scores, std::mt19937& gen);

} // namespace utils

#endif // UTILS_HPP