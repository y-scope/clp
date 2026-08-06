#include <chrono>
#include <string>
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <utils/profiling/Profiler.hpp>
#include <utils/profiling/Reporter.hpp>
#include <utils/profiling/test/sinks.hpp>

namespace utils::profiling::test {
TEST_CASE("reporter_emits_on_destruction", "[Reporter]") {
    Reporter<VerifyingSink> const reporter{"test", "test.reporter_test", 1U};
    Profiler::start_measurement("reporter_test");
    std::this_thread::sleep_for(std::chrono::milliseconds(cSleepMsShort));
    Profiler::stop_measurement("reporter_test");
}

TEST_CASE("reporter_emits_only_measurements_with_calls", "[Reporter]") {
    int emit_count{0};
    std::string emitted_name;

    {
        Reporter<CountingSink> const reporter{"test", &emit_count, &emitted_name};
        Profiler::start_measurement("with_calls");
        Profiler::stop_measurement("with_calls");
        // "no_calls" is started but never stopped, so it doesn't emit and increase the count.
        Profiler::start_measurement("no_calls");
    }

    REQUIRE(1 == emit_count);
    REQUIRE("test.with_calls" == emitted_name);
}

TEST_CASE("nested_reporters_emit_independently", "[Reporter]") {
    int outer_count{0};
    int inner_count{0};
    std::string outer_name;
    std::string inner_name;

    Reporter<CountingSink> const outer{"outer", &outer_count, &outer_name};
    Profiler::start_measurement("outer_work");
    std::this_thread::sleep_for(std::chrono::milliseconds(cSleepMsShort));
    Profiler::stop_measurement("outer_work");

    {
        Reporter<CountingSink> const inner{"inner", &inner_count, &inner_name};
        Profiler::start_measurement("inner_work");
        std::this_thread::sleep_for(std::chrono::milliseconds(cSleepMsShort));
        Profiler::stop_measurement("inner_work");
    }

    // Inner reporter has emitted — outer has not yet.
    REQUIRE(1 == inner_count);
    REQUIRE(0 == outer_count);
    REQUIRE("outer.inner.inner_work" == inner_name);
}

TEST_CASE("nested_reporters_same_scope_name_no_collision", "[Reporter]") {
    int outer_count{0};
    int inner_count{0};
    std::string outer_name;
    std::string inner_name;

    Reporter<CountingSink> const outer{"parent", &outer_count, &outer_name};
    Profiler::start_measurement("work");
    Profiler::stop_measurement("work");

    {
        Reporter<CountingSink> const inner{"child", &inner_count, &inner_name};
        Profiler::start_measurement("work");
        Profiler::stop_measurement("work");
    }

    REQUIRE(1 == inner_count);
    REQUIRE("parent.child.work" == inner_name);
}
}  // namespace utils::profiling::test
