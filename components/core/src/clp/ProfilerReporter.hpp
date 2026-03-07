#ifndef CLP_PROFILER_REPORT_HPP
#define CLP_PROFILER_REPORT_HPP

#include <string>
#include <unordered_map>

#include <spdlog/spdlog.h>

#include "Profiler.hpp"
#include "Stopwatch.hpp"

namespace clp {
/**
 * RAII helper for automatically reporting all runtime measurements at scope exit.
 *
 * This class is designed to simplify the reporting of runtime measurements captured using the
 * `Profiler` class. By creating an instance of `ProfilerReporter`, all runtime measurements will
 * automatically be printed when the object goes out of scope.
 *
 * Usage:
 * - Define a `ProfilerReporter` at any logical unit that encompasses all operations you want to
 *   profile. A common place is in the `main()` function of an executables.
 * - If `ScopedProfiler` or `Profiler` is used in the same scope as `ProfilerReporter`,
 *   `ProfilerReporter`must be declared first, such that its destructor is called last.
 * - Once the object is destructured (scope exit), the runtime measurmenets are reported.
 *
 * Notes:
 *  - Only runtime measurements (those tracked by string names) are reported. This class is
 *    primarily designed to work in tandem with the `ScopedProfiler` class.
 *  - Copy and move operations are removed to prevent accidentaly multiple reporting.
 */
class ProfilerReporter {
public:
    ProfilerReporter() = default;
    explicit ProfilerReporter(std::unordered_map<std::string, Stopwatch>& sink) : m_sink(&sink) {}

    ~ProfilerReporter() {
        if (nullptr != m_sink) {
            *m_sink = Profiler::get_runtime_measurements();
            return;
        }

        SPDLOG_INFO("---MEASUREMENTS START---");
        for (auto const& [name, stopwatch] : Profiler::get_runtime_measurements()) {
            auto total{stopwatch.get_time_taken_in_seconds()};
            auto calls{stopwatch.get_call_count()};
            auto avg{calls > 0 ? total / calls : 0.0};

            SPDLOG_INFO("{}: total {:.3f} s | calls {} | avg {:.3f} s", name, total, calls, avg);
        }
        SPDLOG_INFO("----MEASUREMENTS END----");
    }

    ProfilerReporter(ProfilerReporter const&) = delete;
    ProfilerReporter& operator=(ProfilerReporter const&) = delete;
    ProfilerReporter(ProfilerReporter&&) = delete;
    ProfilerReporter& operator=(ProfilerReporter&&) = delete;

private:
    std::unordered_map<std::string, Stopwatch>* m_sink{nullptr};
};
}  // namespace clp

#endif  // CLP_PROFILER_REPORT_HPP
