#ifndef UTILS_PROFILING_TEST_EMITTERS_HPP
#define UTILS_PROFILING_TEST_EMITTERS_HPP

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>
#include <utils/profiling/Stopwatch.hpp>

namespace utils::profiling::test {
inline constexpr int cSleepMsShort{10};
inline constexpr int cSleepMsMedium{50};
inline constexpr int cSleepMsLong{100};

/**
 * Returns an emit callback that asserts the measurement name, call count, and a minimum
 * duration of 1ms.
 */
inline auto verify_emit(std::string_view expected_name, uint32_t expected_call_count) {
    return [expected_name,
            expected_call_count](std::string_view name, Measurement measurement) -> void {
        REQUIRE(expected_name == name);
        REQUIRE(expected_call_count == measurement.call_count);
        REQUIRE(measurement.duration >= std::chrono::milliseconds(1));
    };
}

/**
 * Returns an emit callback that increments a counter and records the last emitted name.
 */
inline auto counting_emit(int& emit_count, std::string& last_name) {
    return [&emit_count, &last_name](std::string_view name, Measurement) -> void {
        ++emit_count;
        last_name = std::string{name};
    };
}
}  // namespace utils::profiling::test

#endif  // UTILS_PROFILING_TEST_EMITTERS_HPP
