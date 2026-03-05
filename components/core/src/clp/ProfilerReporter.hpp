#ifndef CLP_PROFILER_REPORT_HPP
#define CLP_PROFILER_REPORT_HPP

#include "Profiler.hpp"
#include <spdlog/spdlog.h>

namespace clp {
class ProfilerReporter {
public:
    ProfilerReporter() { Profiler::init(); }
    ~ProfilerReporter() { print_all_enabled_measurements(); }

    ProfilerReporter(const ProfilerReporter&) = delete;
    ProfilerReporter& operator=(const ProfilerReporter&) = delete;
    ProfilerReporter(ProfilerReporter&&) = delete;
    ProfilerReporter& operator=(ProfilerReporter&&) = delete;

    auto print_all_enabled_measurements() -> void {
        auto length{enum_to_underlying_type(Profiler::FragmentedMeasurementIndex::Length)};
        for (size_t i{0}; i < length; ++i) {
            if (Profiler::cFragmentedMeasurementEnabled[i]) {
                auto index{static_cast<Profiler::FragmentedMeasurementIndex>(i)};
                auto runtime{Profiler::get_fragmented_measurement_in_seconds_runtime(index)};
                SPDLOG_INFO("Measurement {}: {} s", i, runtime);
            }
        }
    }
};
}  // namespace clp

#endif  // CLP_PROFILER_REPORT_HPP
