#include <chrono>
#include <string>
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <utils/profiling/Reporter.hpp>
#include <utils/profiling/ScopedProfiler.hpp>
#include <utils/profiling/test/emitters.hpp>

namespace utils::profiling::test {
TEST_CASE("scoped_profiler_starts_and_stops_measurement", "[ScopedProfiler]") {
    auto emit{verify_emit("test.test_scope", 1U)};
    Reporter const reporter{"test", emit};
    {
        ScopedProfiler const scoped{"test_scope"};
        std::this_thread::sleep_for(std::chrono::milliseconds(cSleepMsMedium));
    }
}

TEST_CASE("nested_scoped_profilers_accumulate_separately", "[ScopedProfiler]") {
    int emit_count{0};
    std::string last_name;
    auto emit{counting_emit(emit_count, last_name)};
    {
        Reporter const reporter{"test", emit};
        {
            ScopedProfiler const outer{"outer"};
            std::this_thread::sleep_for(std::chrono::milliseconds(cSleepMsShort));
            {
                ScopedProfiler const inner{"inner"};
                std::this_thread::sleep_for(std::chrono::milliseconds(cSleepMsShort));
            }
        }
    }
    REQUIRE(2 == emit_count);
}
}  // namespace utils::profiling::test
