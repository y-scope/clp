#ifndef CLP_PROFILER_REPORT_HPP
#define CLP_PROFILER_REPORT_HPP

#include "Profiler.hpp"

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
    ProfilerReporter() { Profiler::init(); }
    ~ProfilerReporter() { Profiler::print_all_runtime_measurements(); }

    ProfilerReporter(const ProfilerReporter&) = delete;
    ProfilerReporter& operator=(const ProfilerReporter&) = delete;
    ProfilerReporter(ProfilerReporter&&) = delete;
    ProfilerReporter& operator=(ProfilerReporter&&) = delete;
};
}  // namespace clp

#endif  // CLP_PROFILER_REPORT_HPP
