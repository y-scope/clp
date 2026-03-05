#include <chrono>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include <clp/Profiler.hpp>
#include <clp/ScopedProfiler.hpp>
#include <clp/type_utils.hpp>

using clp::Profiler;
using clp::Profiler;
using clp::ScopedProfiler;

constexpr auto cName{"ProfileName"};

TEST_CASE("macro_is_set", "[profiler]") {
    REQUIRE(PROF_ACTIVE == 1);
}

TEST_CASE("scoped_profiler_starts_and_stops_timer_automatically", "[profiler]") {
    Profiler::init();
    Profiler::reset_runtime_measurement(cName);

    {
        ScopedProfiler profiler(cName);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    auto const measured{Profiler::get_runtime_measurement_in_seconds(cName)};
    REQUIRE(measured >= 0.05);
    REQUIRE(measured < 0.14);
}

TEST_CASE("scoped_profiler_accumulates_across_multiple_scopes", "[profiler]") {
    Profiler::init();
    Profiler::reset_runtime_measurement(cName);

    {
        ScopedProfiler profiler(cName);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    {
        ScopedProfiler profiler(cName);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    auto const measured{Profiler::get_runtime_measurement_in_seconds(cName)};
    REQUIRE(measured >= 0.05);
    REQUIRE(measured < 0.14);
}

TEST_CASE("scoped_profiler_macro_works", "[profiler]") {
    Profiler::init();
    Profiler::reset_runtime_measurement(cName);

    {
        PROFILE_SCOPE(cName);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    auto const measured{Profiler::get_runtime_measurement_in_seconds(cName)};
    REQUIRE(measured >= 0.05);
    REQUIRE(measured < 0.14);
}
