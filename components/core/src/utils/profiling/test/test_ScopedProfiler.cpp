#include <functional>
#include <string>
#include <thread>

#include <catch2/catch_test_macros.hpp>

#undef CLP_ENABLE_PROFILING
// NOLINTNEXTLINE
#define CLP_ENABLE_PROFILING 1

#include <utils/profiling/Reporter.hpp>
#include <utils/profiling/ScopedProfiler.hpp>
#include <utils/profiling/test/emitters.hpp>

namespace utils::profiling::test {
TEST_CASE("scoped_profiler_starts_and_stops_measurement", "[ScopedProfiler]") {
    Reporter const reporter{"test", verify_emit("test.test_scope", 1U)};
    PROFILE_SCOPE("test_scope");
    std::this_thread::sleep_for(cSleep);
}

TEST_CASE("nested_scoped_profilers_accumulate_separately", "[ScopedProfiler]") {
    int emit_count{0};
    std::string last_name;
    {
        Reporter const reporter{"test", counting_emit(emit_count, last_name)};
        {
            PROFILE_SCOPE("outer");
            std::this_thread::sleep_for(cSleep);
            {
                PROFILE_SCOPE("inner");
                std::this_thread::sleep_for(cSleep);
            }
        }
    }
    REQUIRE(2 == emit_count);
}

TEST_CASE("recursive_profile_scope_is_noop", "[ScopedProfiler]") {
    int emit_count{0};
    std::string last_name;
    {
        Reporter const reporter{"test", counting_emit(emit_count, last_name)};
        std::function<void(int)> recursive{[&](int depth) -> void {
            PROFILE_SCOPE("recursive");
            std::this_thread::sleep_for(cSleep);
            if (depth > 0) {
                recursive(depth - 1);
            }
        }};
        recursive(3);
    }
    REQUIRE(1 == emit_count);
    REQUIRE("test.recursive" == last_name);
}
}  // namespace utils::profiling::test
