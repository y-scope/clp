#ifndef CLP_PROFILER_REPORTER_HPP
#define CLP_PROFILER_REPORTER_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>

#include <spdlog/spdlog.h>

#include "Profiler.hpp"
#include "Stopwatch.hpp"

namespace clp {
/**
 * RAII helper for automatically reporting all runtime measurements at scope exit.
 *
 * This class is designed to simplify the reporting of runtime measurements captured using the
 * `Profiler` class.
 *
 * Features:
 * - Prints runtime measurements (total time, call count, average time) when the object is
 *   destructed. This is the default behavior if no sink is provided in the constructor.
 * - Can write measurements to a user provided sink when the object is destructed.
 *   - The sink will be cleared before reporting.
 *   - Printing will not occur if writing to a sink.
 * - Supports disabling certain scopes so neither printed nor written to sink.
 *
 * Usage:
 * - Define a `ProfilerReporter` at any logical unit that encompasses all operations you want to
 *   profile. A common place is in the `main()` function of an executable.
 * - If `ScopedProfiler` or `Profiler` is used in the same scope as `ProfilerReporter`,
 *   `ProfilerReporter` must be declared first, such that its destructor is called last.
 * - Once the object is destructed (scope exit), the runtime measurments are reported.
 *
 * Notes:
 *  - Only runtime measurements (those tracked by string names) are reported. This class is
 *    primarily designed to work in tandem with the `ScopedProfiler` class.
 *  - Copy and move operations are removed to prevent accidental multiple reporting.
 */
class ProfilerReporter {
public:
    explicit ProfilerReporter(std::unordered_set<std::string> disabled_scopes = {}) {
        set_disabled_scopes(std::move(disabled_scopes));
    }

    explicit ProfilerReporter(
            std::unordered_map<std::string, Stopwatch>& sink,
            std::unordered_set<std::string> disabled_scopes = {}
    )
            : m_sink(&sink) {
        set_disabled_scopes(std::move(disabled_scopes));
    }

    ~ProfilerReporter() {
        if (nullptr != m_sink) {
            m_sink->clear();
            for (auto const& [name, stopwatch] : Profiler::get_runtime_measurements()) {
                if (false == m_disabled_scopes.contains(name)) {
                    m_sink->insert({name, stopwatch});
                }
            }
            return;
        }

        SPDLOG_INFO("---MEASUREMENTS START---");
        for (auto const& [name, stopwatch] : Profiler::get_runtime_measurements()) {
            if (m_disabled_scopes.contains(name)) {
                continue;
            }
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
    auto set_disabled_scopes(std::unordered_set<std::string> disabled_scopes) -> void {
        m_disabled_scopes = std::move(disabled_scopes);
    }

    std::unordered_map<std::string, Stopwatch>* m_sink{nullptr};
    std::unordered_set<std::string> m_disabled_scopes;
};
}  // namespace clp

#endif  // CLP_PROFILER_REPORTER_HPP
