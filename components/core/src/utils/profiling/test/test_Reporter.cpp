#include <chrono>
#include <string>
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <utils/profiling/Profiler.hpp>
#include <utils/profiling/Reporter.hpp>
#include <utils/profiling/test/emitters.hpp>

namespace utils::profiling::test {
TEST_CASE("reporter_emits_on_destruction", "[Reporter]") {
    auto emit{verify_emit("test.reporter_test", 1U)};
    Reporter const reporter{"test", emit};
    auto const full_name{Profiler::build_full_name("reporter_test")};
    Profiler::start_measurement(full_name);
    std::this_thread::sleep_for(std::chrono::milliseconds(cSleepMsShort));
    Profiler::stop_measurement(full_name);
}

TEST_CASE("reporter_emits_only_measurements_with_calls", "[Reporter]") {
    int emit_count{0};
    std::string emitted_name;
    auto emit{counting_emit(emit_count, emitted_name)};
    {
        Reporter const reporter{"test", emit};
        auto const full_name{Profiler::build_full_name("with_calls")};
        Profiler::start_measurement(full_name);
        Profiler::stop_measurement(full_name);
        // "no_calls" is started but never stopped, so it doesn't emit and increase the count.
        Profiler::start_measurement(Profiler::build_full_name("no_calls"));
    }
    REQUIRE(1 == emit_count);
    REQUIRE("test.with_calls" == emitted_name);
}

TEST_CASE("nested_reporters_emit_independently", "[Reporter]") {
    int outer_count{0};
    int inner_count{0};
    std::string outer_name;
    std::string inner_name;

    auto outer_emit{counting_emit(outer_count, outer_name)};
    Reporter const outer{"outer", outer_emit};
    auto const outer_name_full{Profiler::build_full_name("outer_work")};
    Profiler::start_measurement(outer_name_full);
    std::this_thread::sleep_for(std::chrono::milliseconds(cSleepMsShort));
    Profiler::stop_measurement(outer_name_full);

    {
        auto inner_emit{counting_emit(inner_count, inner_name)};
        Reporter const inner{"inner", inner_emit};
        auto const inner_name_full{Profiler::build_full_name("inner_work")};
        Profiler::start_measurement(inner_name_full);
        std::this_thread::sleep_for(std::chrono::milliseconds(cSleepMsShort));
        Profiler::stop_measurement(inner_name_full);
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

    auto outer_emit{counting_emit(outer_count, outer_name)};
    Reporter const outer{"parent", outer_emit};
    auto const outer_work{Profiler::build_full_name("work")};
    Profiler::start_measurement(outer_work);
    Profiler::stop_measurement(outer_work);

    {
        auto inner_emit{counting_emit(inner_count, inner_name)};
        Reporter const inner{"child", inner_emit};
        auto const inner_work{Profiler::build_full_name("work")};
        Profiler::start_measurement(inner_work);
        Profiler::stop_measurement(inner_work);
    }

    REQUIRE(1 == inner_count);
    REQUIRE("parent.child.work" == inner_name);
}
}  // namespace utils::profiling::test
