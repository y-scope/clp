#ifndef CLP_SCOPED_PROFILER_HPP
#define CLP_SCOPED_PROFILER_HPP

#include <string>

#include "Profiler.hpp"

namespace clp {
/**
 * RAII wrapper to measure the execution time of a code scope.
 *
 * This class starts a runtime measurement in its constructor and stops it in its destructor. It
 * reports the measured time to the corresponding slot in the profiler. Use this class when you want
 * to measure a single logical phase of your program (e.g., a method) without calling start/stop.
 *
 * Usage for a logical phase:
 * - Use macro PROFILE_SCOPE(`name`) at the top of the logical phase, ideally this is always done
 *   at the top of a method for organization and clarity.
 * - Set `DPROF_ENABLED=1` in `cmakelists`.
 *
 * Notes:
 * - Safe with early returns and exceptions because stopping occurs in the destructor.
 * - All measurements respect `PROF_ENABLED`, so no code is generated when profiling is disabled.
 */
class ScopedProfiler {
public:
    ScopedProfiler(std::string const& name) : m_name(name) {
        Profiler::init();
        Profiler::start_runtime_measurement(name);
    }

    ~ScopedProfiler() { Profiler::stop_runtime_measurement(m_name); }

    ScopedProfiler(const ScopedProfiler&) = delete;
    ScopedProfiler& operator=(const ScopedProfiler&) = delete;
    ScopedProfiler(ScopedProfiler&&) = delete;
    ScopedProfiler& operator=(ScopedProfiler&&) = delete;

private:
    std::string m_name;
};
}  // namespace clp

#define CLP_CONCAT_IMPL(x, y) x##y

#define CLP_CONCAT(x, y) CLP_CONCAT_IMPL(x, y)

#define PROFILE_SCOPE(x) ::clp::ScopedProfiler CLP_CONCAT(__clp_profile_scope_, __LINE__){x}

#endif  // CLP_SCOPED_PROFILER_HPP
