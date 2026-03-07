#include <chrono>
#include <string>
#include <thread>
#include <unordered_map>

#include <catch2/catch_test_macros.hpp>

#include <clp/Profiler.hpp>
#include <clp/ProfilerReporter.hpp>
#include <clp/ScopedProfiler.hpp>
#include <clp/Stopwatch.hpp>

using clp::Profiler;
using clp::ProfilerReporter;
using clp::ScopedProfiler;
using clp::Stopwatch;
using std::string;
using std::unordered_map;

using Sink = unordered_map<string, Stopwatch>;
using ExpectedSink = unordered_map<string, std::pair<double, uint32_t>>;

constexpr double cTimerMarginOfError{0.1};

auto check_sink(Sink const& actual_sink, ExpectedSink const& expected_sink) {
    for (auto const& [name, expected] : expected_sink) {
        auto expected_time{expected.first};
        auto expected_calls{expected.second};

        REQUIRE(actual_sink.contains(name));
        auto const actual_time{actual_sink.at(name).get_time_taken_in_seconds()};
        auto const actual_calls{actual_sink.at(name).get_call_count()};
        REQUIRE(actual_time >= expected_time);
        REQUIRE(actual_time < expected_time + cTimerMarginOfError);
        REQUIRE(actual_calls == expected_calls);
    }
}

TEST_CASE("profiler_reporter_reports_runtime_measurements", "[ProfilerReporter][Stopwatch]") {
    Sink sink0;
    Sink sink1;
    Sink sink2;
    Sink sink3;

    Profiler::reset_runtime_measurements();
    {
        ProfilerReporter profiler0(sink0);
        PROFILE_SCOPE("scope0");

        {
            ProfilerReporter profiler1(sink1);
            PROFILE_SCOPE("scope1");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        {
            ProfilerReporter profiler2(sink2);
            PROFILE_SCOPE("scope2");
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        {
            ProfilerReporter profiler3(sink3);
            PROFILE_SCOPE("scope2");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    check_sink(sink1, {{"scope0", {0, 0}}, {"scope1", {0.01, 1}}});
    check_sink(sink2, {{"scope0", {0, 0}}, {"scope1", {0.01, 1}}, {"scope2", {0.02, 1}}});
    check_sink(sink3, {{"scope0", {0, 0}}, {"scope1", {0.01, 1}}, {"scope2", {0.07, 2}}});
    check_sink(sink0, {{"scope0", {0.18, 1}}, {"scope1", {0.01, 1}}, {"scope2", {0.07, 2}}});
}
