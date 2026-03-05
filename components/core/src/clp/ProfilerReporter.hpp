#ifndef CLP_PROFILER_REPORT_HPP
#define CLP_PROFILER_REPORT_HPP

#include "Profiler.hpp"

namespace clp {
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
