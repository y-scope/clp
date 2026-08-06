#include <chrono>
#include <string>
#include <string_view>
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <utils/profiling/Profiler.hpp>
#include <utils/profiling/Reporter.hpp>
#include <utils/profiling/test/sinks.hpp>

namespace utils::profiling::test {
TEST_CASE("profiler_single_measurement", "[Profiler]") {
    Reporter<VerifyingSink> const reporter{"test", "test.test_single", 1U};
    std::string_view const name{"test_single"};
    Profiler::start_measurement(name);
    std::this_thread::sleep_for(std::chrono::milliseconds(cSleepMsLong));
    Profiler::stop_measurement(name);
}

TEST_CASE("profiler_accumulates_multiple_calls", "[Profiler]") {
    Reporter<VerifyingSink> const reporter{"test", "test.test_accumulate", 2U};
    std::string_view const name{"test_accumulate"};
    Profiler::start_measurement(name);
    std::this_thread::sleep_for(std::chrono::milliseconds(cSleepMsMedium));
    Profiler::stop_measurement(name);

    Profiler::start_measurement(name);
    std::this_thread::sleep_for(std::chrono::milliseconds(cSleepMsMedium));
    Profiler::stop_measurement(name);
}

TEST_CASE("profiler_stop_without_start_is_noop", "[Profiler]") {
    int emit_count{0};
    std::string last_name;

    {
        Reporter<CountingSink> const reporter{"test", &emit_count, &last_name};
        Profiler::stop_measurement("never_started");
    }

    REQUIRE(0 == emit_count);
}

TEST_CASE("profiler_reentrant_start_is_noop", "[Profiler]") {
    Reporter<VerifyingSink> const reporter{"test", "test.test_reentrant", 1U};
    std::string_view const name{"test_reentrant"};
    Profiler::start_measurement(name);
    std::this_thread::sleep_for(std::chrono::milliseconds(cSleepMsMedium));
    Profiler::start_measurement(name);
    std::this_thread::sleep_for(std::chrono::milliseconds(cSleepMsMedium));
    Profiler::stop_measurement(name);
}

TEST_CASE("profiler_noop_without_active_reporter", "[Profiler]") {
    Profiler::start_measurement("no_reporter");
    std::this_thread::sleep_for(std::chrono::milliseconds(cSleepMsShort));
    Profiler::stop_measurement("no_reporter");
}

TEST_CASE("profiler_hierarchical_name_includes_reporter_prefix", "[Profiler]") {
    int parent_emit_count{0};
    std::string parent_last_name;
    Reporter<CountingSink> const parent_reporter{"parent", &parent_emit_count, &parent_last_name};
    Reporter<VerifyingSink> const child_reporter{"child", "parent.child.test", 1U};
    Profiler::start_measurement("test");
    std::this_thread::sleep_for(std::chrono::milliseconds(cSleepMsShort));
    Profiler::stop_measurement("test");
    REQUIRE(0 == parent_emit_count);
}
}  // namespace utils::profiling::test
