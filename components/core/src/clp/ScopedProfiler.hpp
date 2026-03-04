#ifndef CLP_SCOPED_PROFILER_HPP
#define CLP_SCOPED_PROFILER_HPP

#include "Profiler.hpp"

namespace clp {
/**
 * RAII wrapper to measure the execution time of a code scope.
 *
 * This class starts a continuous measurement in its constructor and stops it in its destructor. It
 * reports the measured time to the corresponding slot in the profiler. Use this class when you want
 * to measure a single logical phase of your program (e.g., a method) without calling start/stop.
 *
 * Usage for a logical phase:
 * - Define a unique measurement `index` in `Profiler::ContinuousMeasurementIndex`. Each `index`
 *   corresponds to a slot in the profiler that accumulates total time.
 * - Use macro PROFILE_SCOPE(`index`) at the top of the logical phase, ideally this is always done
 *   at the top of a method for organization and clarity.
 * - Set `DPROF_ENABLED=1` in `cmakelists`.
 *
 * Notes:
 * - Safe with early returns and exceptions because stopping occurs in the destructor.
 * - All measurements respect `PROF_ENABLED`, so no code is generated when profiling is disabled.
 */
template <Profiler::ContinuousMeasurementIndex index>
class ScopedProfiler {
public:
    inline ScopedProfiler() { Profiler::start_continuous_measurement<index>(); }

    inline ~ScopedProfiler() { Profiler::stop_continuous_measurement<index>(); }

    ScopedProfiler(const ScopedProfiler&) = delete;
    ScopedProfiler& operator=(const ScopedProfiler&) = delete;
    ScopedProfiler(ScopedProfiler&&) = delete;
    ScopedProfiler& operator=(ScopedProfiler&&) = delete;
};
}  // namespace clp

#define CLP_CONCAT_IMPL(x, y) x##y

#define CLP_CONCAT(x, y) CLP_CONCAT_IMPL(x, y)

#define PROFILE_SCOPE(x) ::clp::ScopedProfiler<x> CLP_CONCAT(__clp_profile_scope_, __LINE__)

#endif  // CLP_SCOPED_PROFILER_HPP
