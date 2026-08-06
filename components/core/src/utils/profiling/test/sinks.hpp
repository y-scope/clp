#ifndef UTILS_PROFILING_TEST_SINKS_HPP
#define UTILS_PROFILING_TEST_SINKS_HPP

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <utils/profiling/Sink.hpp>
#include <utils/profiling/Stopwatch.hpp>

namespace utils::profiling::test {
inline constexpr int cSleepMsShort{10};
inline constexpr int cSleepMsMedium{50};
inline constexpr int cSleepMsLong{100};

/**
 * Sink that verifies the measurement name, call count, and minimum duration.
 */
struct VerifyingSink : public Sink {
    std::string expected_name;
    uint32_t expected_call_count{0};

    VerifyingSink() = default;

    VerifyingSink(std::string name, uint32_t call_count)
            : expected_name{std::move(name)},
              expected_call_count{call_count} {}

    auto emit(std::string_view name, Measurement measurement) -> void override {
        REQUIRE(expected_name == name);
        REQUIRE(expected_call_count == measurement.call_count);
        REQUIRE(measurement.duration >= std::chrono::milliseconds(1));
    }
};

/**
 * Sink that counts emit calls and records the last measurement name.
 */
struct CountingSink : public Sink {
    int* emit_count;
    std::string* last_name;

    explicit CountingSink(int* c, std::string* name) : emit_count{c}, last_name{name} {}

    auto emit(std::string_view name, Measurement) -> void override {
        ++*emit_count;
        *last_name = std::string{name};
    }
};
}  // namespace utils::profiling::test

#endif  // UTILS_PROFILING_TEST_SINKS_HPP
