#include <chrono>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include <clp/Profiler.hpp>
#include <clp/ScopedProfiler.hpp>
#include <clp/type_utils.hpp>

using clp::Profiler;
using clp::Profiler;
using clp::ScopedProfiler;

constexpr auto cIndex{Profiler::FragmentedMeasurementIndex::Search};

TEST_CASE("macro_is_set", "[profiler]") {
    REQUIRE(PROF_ACTIVE == 1);
}

TEST_CASE("measurement_index_is_set", "[profiler]") {
    REQUIRE(Profiler::cFragmentedMeasurementEnabled[clp::enum_to_underlying_type(cIndex)]);
}

TEST_CASE("scoped_profiler_starts_and_stops_timer_automatically", "[profiler]") {
    Profiler::init();
    Profiler::reset_fragmented_measurement<cIndex>();

    {
        ScopedProfiler<cIndex> profiler;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    auto measured{Profiler::get_fragmented_measurement_in_seconds<cIndex>()};
    REQUIRE(measured >= 0.05);
    REQUIRE(measured < 0.14);
}

TEST_CASE("scoped_profiler_accumulates_across_multiple_scopes", "[profiler]") {
    Profiler::init();
    Profiler::reset_fragmented_measurement<cIndex>();

    {
        ScopedProfiler<cIndex> profiler;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    {
        ScopedProfiler<cIndex> profiler;
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    auto measured{Profiler::get_fragmented_measurement_in_seconds<cIndex>()};
    REQUIRE(measured >= 0.05);
    REQUIRE(measured < 0.14);
}

TEST_CASE("scoped_profiler_macro_works", "[profiler]") {
    Profiler::init();
    Profiler::reset_fragmented_measurement<cIndex>();

    {
        PROFILE_SCOPE(cIndex);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    auto measured{Profiler::get_fragmented_measurement_in_seconds<cIndex>()};
    REQUIRE(measured >= 0.05);
    REQUIRE(measured < 0.14);
}
