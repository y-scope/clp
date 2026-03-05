#include <catch2/catch_test_macros.hpp>

#include <clp/ProfilerReporter.hpp>

using clp::ProfilerReporter;

TEST_CASE("create_profiler_repoter", "[profiler]") {
    ProfilerReporter profiler_reporter;
}