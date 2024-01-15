#ifndef CLP_PROFILER_HPP
#define CLP_PROFILER_HPP

#include <array>
#include <vector>

#include "Stopwatch.hpp"
#include "type_utils.hpp"

namespace clp {
/**
 * Class to time code.
 *
 * There are two types of measurements:
 * - Continuous measurements where a user needs to time a single, continuous operation.
 * - Fragmented measurements where a user needs to time multiple, separated instances of an
 *   operation. For example if we want to get the total run time taken for inserting entries into a
 *   dictionary, we could wrap the insertion with a fragmented measurement.
 *
 * To add a measurement, add it to the ContinuousMeasurementIndex or FragmentedMeasurementIndex
 * enums and add a corresponding enable flag to cContinuousMeasurementEnabled or
 * cFragmentedMeasurementEnabled. The flags allow enabling/disabling specific measurements such that
 * a disabled measurement will not affect the performance of the program (except for extra heap
 * storage).
 *
 * To log a measurement, use LOG_CONTINUOUS_MEASUREMENT or LOG_FRAGMENTED_MEASUREMENT, passing in
 * the relevant measurement index enum.
 *
 * Two implementation details allow this class to avoid inducing overhead when profiling is
 * disabled:
 * - All methods bodies are defined in the header, guarded by `if constexpr (PROF_ENABLED)`. When
 *   profiling is disabled, the compiler will detect the empty body and won't add any code to the
 *   binary; if the methods were instead defined in the .cpp file, the compiler would still generate
 *   an empty method.
 * - The methods use the measurement enum as a template parameter to indicate which measurement the
 *   method call is for. So at compile-time, for each measurement, the compiler can use the enable
 *   flag to determine whether to generate code to do the measurement or whether to do nothing.
 */
class Profiler {
public:
    // Types
    enum class ContinuousMeasurementIndex : size_t {
        Compression = 0,
        ParseLogFile,
        Search,
        Length
    };
    enum class FragmentedMeasurementIndex : size_t {
        Length
    };

    // Constants
    // NOTE: We use lambdas so that we can programmatically initialize the constexpr array
    static constexpr auto cContinuousMeasurementEnabled = []() {
        std::array<bool, enum_to_underlying_type(ContinuousMeasurementIndex::Length)> enabled{};
        enabled[enum_to_underlying_type(ContinuousMeasurementIndex::Compression)] = true;
        enabled[enum_to_underlying_type(ContinuousMeasurementIndex::ParseLogFile)] = true;
        enabled[enum_to_underlying_type(ContinuousMeasurementIndex::Search)] = true;
        return enabled;
    }();
    static constexpr auto cFragmentedMeasurementEnabled = []() {
        std::array<bool, enum_to_underlying_type(FragmentedMeasurementIndex::Length)> enabled{};
        return enabled;
    }();

    // Methods
    /**
     * Static initializer for class. This must be called before using the class.
     */
    static void init() {
        if constexpr (PROF_ENABLED) {
            m_continuous_measurements = new std::vector<Stopwatch>(
                    enum_to_underlying_type(ContinuousMeasurementIndex::Length)
            );
            m_fragmented_measurements = new std::vector<Stopwatch>(
                    enum_to_underlying_type(FragmentedMeasurementIndex::Length)
            );
        }
    }

    template <ContinuousMeasurementIndex index>
    static void start_continuous_measurement() {
        if constexpr (PROF_ENABLED && cContinuousMeasurementEnabled[enum_to_underlying_type(index)])
        {
            auto& stopwatch = (*m_continuous_measurements)[enum_to_underlying_type(index)];
            stopwatch.reset();
            stopwatch.start();
        }
    }

    template <ContinuousMeasurementIndex index>
    static void stop_continuous_measurement() {
        if constexpr (PROF_ENABLED && cContinuousMeasurementEnabled[enum_to_underlying_type(index)])
        {
            (*m_continuous_measurements)[enum_to_underlying_type(index)].stop();
        }
    }

    template <ContinuousMeasurementIndex index>
    static double get_continuous_measurement_in_seconds() {
        if constexpr (PROF_ENABLED) {
            return (*m_continuous_measurements)[enum_to_underlying_type(index)]
                    .get_time_taken_in_seconds();
        } else {
            return 0;
        }
    }

    template <FragmentedMeasurementIndex index>
    static void start_fragmented_measurement() {
        if constexpr (PROF_ENABLED && cFragmentedMeasurementEnabled[enum_to_underlying_type(index)])
        {
            (*m_fragmented_measurements)[enum_to_underlying_type(index)].start();
        }
    }

    template <FragmentedMeasurementIndex index>
    static void stop_fragmented_measurement() {
        if constexpr (PROF_ENABLED && cFragmentedMeasurementEnabled[enum_to_underlying_type(index)])
        {
            (*m_fragmented_measurements)[enum_to_underlying_type(index)].stop();
        }
    }

    template <FragmentedMeasurementIndex index>
    static void reset_fragmented_measurement() {
        if constexpr (PROF_ENABLED && cFragmentedMeasurementEnabled[enum_to_underlying_type(index)])
        {
            (*m_fragmented_measurements)[enum_to_underlying_type(index)].reset();
        }
    }

    template <FragmentedMeasurementIndex index>
    static double get_fragmented_measurement_in_seconds() {
        if constexpr (PROF_ENABLED) {
            return (*m_fragmented_measurements)[enum_to_underlying_type(index)]
                    .get_time_taken_in_seconds();
        } else {
            return 0;
        }
    }

private:
    static std::vector<Stopwatch>* m_fragmented_measurements;
    static std::vector<Stopwatch>* m_continuous_measurements;
};
}  // namespace clp

// Macros to log the measurements
// NOTE: We use macros so that we can add the measurement index to the log (not easy to do with
// templates).
#define LOG_CONTINUOUS_MEASUREMENT(x) \
    if (PROF_ENABLED \
        && ::clp::Profiler::cContinuousMeasurementEnabled[enum_to_underlying_type(x)]) { \
        SPDLOG_INFO( \
                "{} took {} s", \
                #x, \
                ::clp::Profiler::get_continuous_measurement_in_seconds<x>() \
        ); \
    }
#define LOG_FRAGMENTED_MEASUREMENT(x) \
    if (PROF_ENABLED \
        && ::clp::Profiler::cFragmentedMeasurementEnabled[enum_to_underlying_type(x)]) { \
        SPDLOG_INFO( \
                "{} took {} s", \
                #x, \
                ::clp::Profiler::get_fragmented_measurement_in_seconds<x>() \
        ); \
    }
#define PROFILER_SPDLOG_INFO(...) \
    if (PROF_ENABLED) { \
        SPDLOG_INFO(__VA_ARGS__); \
    }

#endif  // CLP_PROFILER_HPP
