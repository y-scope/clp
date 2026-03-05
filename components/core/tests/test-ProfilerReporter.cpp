#include <chrono>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#include <clp/Profiler.hpp>
#include <clp/ProfilerReporter.hpp>
#include <clp/ScopedProfiler.hpp>

using clp::Profiler;
using clp::ProfilerReporter;
using clp::ScopedProfiler;

TEST_CASE("profiler_reporter_reports_runtime_measurements", "[profiler]") {
    Profiler::reset_runtime_measurements();
    {
        ProfilerReporter outter_profiler_reporter;
        PROFILE_SCOPE("scope0");

        {
            PROFILE_SCOPE("scope1");
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            ProfilerReporter inner_profiler_reporter;
        }

        {
            PROFILE_SCOPE("scope2");
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            ProfilerReporter inner_profiler_reporter;
        }

        {
            PROFILE_SCOPE("scope3");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            ProfilerReporter inner_profiler_reporter;
        }

        {
            PROFILE_SCOPE("scope3");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ProfilerReporter inner_profiler_reporter;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}