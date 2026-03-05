#ifndef CLP_PROFILER_HPP
#define CLP_PROFILER_HPP

#if defined(PROF_ENABLED) || defined(PROF_TEST_ENABLED)
    #define PROF_ACTIVE 1
#else
    #define PROF_ACTIVE 0
#endif

#include <array>
#include <unordered_map>
#include <vector>

#include <spdlog/spdlog.h>

#include "Stopwatch.hpp"
#include "type_utils.hpp"

namespace clp {
/**
 * Class to time code.
 *
 * A Measurement can be taken over a single continuous operation, or called multiple times to
 * accumulate fragemented measurements into a single total run time.
 *
 * There are two ways to add a measurement:
 * 1. For measurements that are taken a small number of times use a runtime measurement.
 * 2. For measurements that are in hot loops, use a compile-time measurement, such that when it is
 *    disabled it has zero-overhead. In this case to add a measurement, add it to the
 *    MeasurementIndex enum and add a corresponding enable flag to cMeasurementEnabled. The flags
 *    allow enabling/disabling specific measurements such that a disabled measurement will not
 *    affect the performance of the program (except for extra heap storage).
 *
 * Two implementation details allow this class to avoid inducing overhead when profiling is
 * disabled:
 * - All methods bodies are defined in the header, guarded by `if constexpr (PROF_ACTIVE)`. When
 *   profiling is disabled, the compiler will detect the empty body and won't add any code to the
 *   binary; if the methods were instead defined in the .cpp file, the compiler would still generate
 *   an empty method.
 * - The compile-time methods use the measurement enum as a template parameter to indicate which
 *   measurement the method call is for. So at compile-time, for each measurement, the compiler can
 *   use the enable flag to determine whether to generate code to do the measurement or whether to
 *   do nothing.
 */
class Profiler {
public:
    enum class CompileTimeMeasurementIndex : uint8_t {
        Length
    };

    // NOTE: We use lambdas so that we can programmatically initialize the constexpr array
    static constexpr auto cMeasurementEnabled = []() {
        std::array<bool, enum_to_underlying_type(CompileTimeMeasurementIndex::Length)> enabled{};
        return enabled;
    }();

    // Methods
    /**
     * Static initializer for class. This must be called before using the class.
     */
    static void init() {
        if constexpr (PROF_ACTIVE) {
            if (m_initialized) {
                return;
            }
            m_initialized = true;
            m_compile_time_measurements = new std::vector<Stopwatch>(
                    enum_to_underlying_type(CompileTimeMeasurementIndex::Length)
            );
        }
    }

    static auto start_runtime_measurement(std::string const& name) -> void {
        if constexpr (PROF_ACTIVE) {
            // implicitly creates the timer if it doesn't exist yet
            m_runtime_measurements[name].start();
        }
    }

    static auto stop_runtime_measurement(std::string const& name) -> void {
        if constexpr (PROF_ACTIVE) {
            m_runtime_measurements[name].stop();
        }
    }

    static auto reset_runtime_measurement(std::string const& name) -> void {
        if constexpr (PROF_ACTIVE) {
            m_runtime_measurements[name].reset();
        }
    }

    static auto get_runtime_measurement_in_seconds(std::string const& name) -> double {
        if constexpr (PROF_ACTIVE) {
            return m_runtime_measurements[name].get_time_taken_in_seconds();
        } else {
            return 0;
        }
    }

    static auto print_all_runtime_measurements() -> void {
        for (auto const& [name, stopwatch] : m_runtime_measurements) {
            SPDLOG_INFO("Measurement {}: {} s", name, get_runtime_measurement_in_seconds(name));
        }
    }

    template <CompileTimeMeasurementIndex index>
    static void start_compile_time_measurement() {
        if constexpr (PROF_ACTIVE && cMeasurementEnabled[enum_to_underlying_type(index)])
        {
            (*m_compile_time_measurements)[enum_to_underlying_type(index)].start();
        }
    }

    template <CompileTimeMeasurementIndex index>
    static void stop_compile_time_measurement() {
        if constexpr (PROF_ACTIVE && cMeasurementEnabled[enum_to_underlying_type(index)])
        {
            (*m_compile_time_measurements)[enum_to_underlying_type(index)].stop();
        }
    }

    template <CompileTimeMeasurementIndex index>
    static void reset_compile_time_measurement() {
        if constexpr (PROF_ACTIVE && cMeasurementEnabled[enum_to_underlying_type(index)])
        {
            (*m_compile_time_measurements)[enum_to_underlying_type(index)].reset();
        }
    }

    template <CompileTimeMeasurementIndex index>
    static double get_compile_time_measurement_in_seconds() {
        if constexpr (PROF_ACTIVE && cMeasurementEnabled[enum_to_underlying_type(index)]) {
            return (*m_compile_time_measurements)[enum_to_underlying_type(index)]
                    .get_time_taken_in_seconds();
        } else {
            return 0;
        }
    }

private:
    static std::unordered_map<std::string, Stopwatch> m_runtime_measurements;
    static std::vector<Stopwatch>* m_compile_time_measurements;
    static bool m_initialized;
};
}  // namespace clp
#endif  // CLP_PROFILER_HPP
