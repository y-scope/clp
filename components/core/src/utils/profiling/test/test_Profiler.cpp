#include <chrono>
#include <string>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#undef CLP_ENABLE_PROFILING
#define CLP_ENABLE_PROFILING 1

#include <utils/profiling/Profiler.hpp>
#include <utils/profiling/Reporter.hpp>
#include <utils/profiling/test/emitters.hpp>

namespace utils::profiling::test {
TEST_CASE("profiler_single_measurement", "[Profiler]") {
    auto emit{verify_emit("test.test_single", 1U)};
    Reporter const reporter{"test", emit};
    auto const full_name{Profiler::build_full_name("test_single")};
    Profiler::start_measurement(full_name);
    std::this_thread::sleep_for(cSleep);
    Profiler::stop_measurement(full_name);
}

TEST_CASE("profiler_accumulates_multiple_calls", "[Profiler]") {
    auto emit{verify_emit("test.test_accumulate", 2U)};
    Reporter const reporter{"test", emit};
    auto const full_name{Profiler::build_full_name("test_accumulate")};
    Profiler::start_measurement(full_name);
    std::this_thread::sleep_for(cSleep);
    Profiler::stop_measurement(full_name);

    Profiler::start_measurement(full_name);
    std::this_thread::sleep_for(cSleep);
    Profiler::stop_measurement(full_name);
}

TEST_CASE("profiler_stop_without_start_is_noop", "[Profiler]") {
    int emit_count{0};
    std::string last_name;
    auto emit{counting_emit(emit_count, last_name)};
    {
        Reporter const reporter{"test", emit};
        Profiler::stop_measurement("test.never_started");
    }
    REQUIRE(0 == emit_count);
}

TEST_CASE("profiler_reentrant_start_is_noop", "[Profiler]") {
    auto emit{verify_emit("test.test_reentrant", 1U)};
    Reporter const reporter{"test", emit};
    auto const full_name{Profiler::build_full_name("test_reentrant")};
    Profiler::start_measurement(full_name);
    std::this_thread::sleep_for(cSleep);
    Profiler::start_measurement(full_name);
    std::this_thread::sleep_for(cSleep);
    Profiler::stop_measurement(full_name);
}

TEST_CASE("profiler_hierarchical_name_includes_reporter_prefix", "[Profiler]") {
    int parent_emit_count{0};
    std::string parent_last_name;
    auto parent_emit{counting_emit(parent_emit_count, parent_last_name)};
    Reporter const parent_reporter{"parent", parent_emit};

    auto child_emit{verify_emit("parent.child.test", 1U)};
    Reporter const child_reporter{"child", child_emit};
    auto const full_name{Profiler::build_full_name("test")};
    Profiler::start_measurement(full_name);
    std::this_thread::sleep_for(cSleep);
    Profiler::stop_measurement(full_name);
    REQUIRE(0 == parent_emit_count);
}
}  // namespace utils::profiling::test
