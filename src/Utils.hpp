#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

#ifndef UTILS_HPP
#define UTILS_HPP

namespace utils {

template <typename T> bool contains(const std::vector<T>& c, T elem) {
    return std::find(begin(c), end(c), elem) != end(c);
}

std::size_t roullette(const std::vector<float>& scores, std::mt19937& gen);

class ScopedTimeMeasurement final {
  public:
    explicit ScopedTimeMeasurement(std::string step_description);

    ~ScopedTimeMeasurement();

  private:
    std::string                                step_description;
    decltype(std::chrono::steady_clock::now()) begin;
};

// If the env variable is set, create an object that will measure time and print it when going out
// of scope. Do nothing otherwise.
inline std::unique_ptr<ScopedTimeMeasurement> scoped_time_measurement(std::string description) {
    auto value = std::getenv("TIME_MEASUREMENTS");
    if (value == nullptr || std::string(value) != "1") {
        return nullptr;
    }

    return std::make_unique<ScopedTimeMeasurement>(std::move(description));
}

} // namespace utils

#endif // UTILS_HPP