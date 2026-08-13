#include <string>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#undef CLP_ENABLE_PROFILING
// NOLINTNEXTLINE
#define CLP_ENABLE_PROFILING 1

#include <utils/profiling/Profiler.hpp>
#include <utils/profiling/Reporter.hpp>
#include <utils/profiling/test/emitters.hpp>

namespace utils::profiling::test {
TEST_CASE("reporter_emits_on_destruction", "[Reporter]") {
    Reporter const reporter{"test", verify_emit("test.reporter_test", 1U)};
    auto const full_name{Profiler::build_full_name("reporter_test")};
    Profiler::start_measurement(full_name);
    std::this_thread::sleep_for(cSleep);
    Profiler::stop_measurement(full_name);
}

TEST_CASE("reporter_emits_only_measurements_with_calls", "[Reporter]") {
    int emit_count{0};
    std::string emitted_name;
    {
        Reporter const reporter{"test", counting_emit(emit_count, emitted_name)};
        auto const full_name{Profiler::build_full_name("with_calls")};
        Profiler::start_measurement(full_name);
        Profiler::stop_measurement(full_name);
        // "no_calls" is started but never stopped, so it doesn't emit and increase the count.
        Profiler::start_measurement(Profiler::build_full_name("no_calls"));
    }
    REQUIRE(1 == emit_count);
    REQUIRE("test.with_calls" == emitted_name);
}

TEST_CASE("nested_reporters_emit", "[Reporter]") {
    int outer_count{0};
    int inner_count{0};
    std::string outer_name;
    std::string inner_name;

    Reporter const outer{"parent", counting_emit(outer_count, outer_name)};
    auto const outer_work{Profiler::build_full_name("work")};
    Profiler::start_measurement(outer_work);
    Profiler::stop_measurement(outer_work);

    {
        Reporter const inner{"child", counting_emit(inner_count, inner_name)};
        auto const inner_work{Profiler::build_full_name("work")};
        Profiler::start_measurement(inner_work);
        Profiler::stop_measurement(inner_work);
    }

    REQUIRE(1 == inner_count);
    REQUIRE("parent.child.work" == inner_name);
    // Outer reporter has not emitted yet.
    REQUIRE(0 == outer_count);
}
}  // namespace utils::profiling::test
