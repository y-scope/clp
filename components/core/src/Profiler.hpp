#ifndef PROFILER_HPP
#define PROFILER_HPP

// C++ libraries
#include <atomic>
#include <memory>
#include <vector>

// Project headers
#include "Stopwatch.hpp"

/**
 * Class to perform global profiling of any program in the package.
 * There are two types of measurements:
 * - Fragmented measurements where a user needs to time a multiple, separated instances of an operation.
 * - Continuous measurements where a user needs to time a single, continuous operation.
 *
 * To add an operation, add it to the relevant enum and add a flag indicating whether it's enabled to the relevant flags enum.
 *
 * Measurements that are disabled will not affect the performance of the program (except for extra heap storage) so long as the user uses the profiling
 * preprocessor macros below.
 */
class Profiler {
public:
    // Types
    enum class FragmentedMeasurementIndex : size_t {
        SegmentRead = 0,
        LogtypeDictRead,
        VarDictRead,
        MessageSearch,
        SegmentIndexRead,
        ColumnsAlloc,
        Length
    };
    enum class FragmentedMeasurementEnabled : bool {
        SegmentRead = true,
        LogtypeDictRead = true,
        VarDictRead = true,
        MessageSearch = true,
        SegmentIndexRead = true,
        ColumnsAlloc = true,
    };
    enum class ContinuousMeasurementIndex : size_t {
        PipelineRequest = 0,
        Length
    };
    enum class ContinuousMeasurementEnabled : bool {
        PipelineRequest = true,
    };

    // Methods
    /**
     * Static initializer for class. This must be called before using the class.
     */
    static void init ();

    static void start_continuous_measurement (ContinuousMeasurementIndex index) {
        (*m_continuous_measurement_durations_ns)[(size_t)index] = 0;
        start_measurement(m_continuous_measurement_begin_timestamps_ns, (size_t)index);
    }

    static void stop_continuous_measurement (ContinuousMeasurementIndex index) {
        stop_measurement(m_continuous_measurement_begin_timestamps_ns, m_continuous_measurement_durations_ns, (size_t)index);
    }

    static uint64_t get_continuous_measurement_begin_ns (ContinuousMeasurementIndex index) {
        return (*m_continuous_measurement_begin_timestamps_ns)[(size_t)index].load();
    }

    static uint64_t get_continuous_measurement_duration_ns (ContinuousMeasurementIndex index) {
        return (*m_continuous_measurement_durations_ns)[(size_t)index].load();
    }

    static void start_fragmented_measurement (FragmentedMeasurementIndex index) {
        start_measurement(m_fragmented_measurement_begin_timestamps_ns, (size_t)index);
    }

    static void stop_fragmented_measurement (FragmentedMeasurementIndex index) {
        stop_measurement(m_fragmented_measurement_begin_timestamps_ns, m_fragmented_measurement_durations_ns, (size_t)index);
    }

    static void reset_fragmented_measurement (FragmentedMeasurementIndex index) {
        (*m_fragmented_measurement_durations_ns)[(size_t)index] = 0;
    }

    static void increment_fragmented_measurement (FragmentedMeasurementIndex index, uint64_t duration_ns) {
        (*m_fragmented_measurement_durations_ns)[(size_t)index] += duration_ns;
    }

    static uint64_t get_fragmented_measurement (FragmentedMeasurementIndex index) {
        return (*m_fragmented_measurement_durations_ns)[(size_t)index].load();
    }

private:
    // Methods
    static void start_measurement (std::unique_ptr<std::vector<std::atomic_uint64_t>>& begin_timestamps_in_nanoseconds, size_t index);
    static void stop_measurement (std::unique_ptr<std::vector<std::atomic_uint64_t>>& begin_timestamps_in_nanoseconds,
                                  std::unique_ptr<std::vector<std::atomic_uint64_t>>& durations_in_nanoseconds, size_t index);

    // Variables
    static std::unique_ptr<std::vector<std::atomic_uint64_t>> m_fragmented_measurement_begin_timestamps_ns;
    static std::unique_ptr<std::vector<std::atomic_uint64_t>> m_fragmented_measurement_durations_ns;
    static std::unique_ptr<std::vector<std::atomic_uint64_t>> m_continuous_measurement_begin_timestamps_ns;
    static std::unique_ptr<std::vector<std::atomic_uint64_t>> m_continuous_measurement_durations_ns;
};

// Profiling macros
#define PROFILER_INITIALIZE() if (PROF_ENABLED) { \
    Profiler::init(); \
}

#define PROFILER_FRAGMENTED_MEASUREMENT_START(NAME) \
    if (PROF_ENABLED && (bool)Profiler::FragmentedMeasurementEnabled::NAME) { \
        Profiler::start_fragmented_measurement(Profiler::FragmentedMeasurementIndex::NAME); \
    }
#define PROFILER_FRAGMENTED_MEASUREMENT_STOP(NAME) \
    if (PROF_ENABLED && (bool)Profiler::FragmentedMeasurementEnabled::NAME) { \
        Profiler::stop_fragmented_measurement(Profiler::FragmentedMeasurementIndex::NAME); \
    }
#define PROFILER_FRAGMENTED_MEASUREMENT_RESET(NAME) \
    if (PROF_ENABLED && (bool)Profiler::FragmentedMeasurementEnabled::NAME) { \
        Profiler::reset_fragmented_measurement(Profiler::FragmentedMeasurementIndex::NAME); \
    }
#define PROFILER_FRAGMENTED_MEASUREMENT_INCREMENT(NAME, DURATION_NS) \
    if (PROF_ENABLED && (bool)Profiler::FragmentedMeasurementEnabled::NAME) { \
        Profiler::increment_fragmented_measurement(Profiler::FragmentedMeasurementIndex::NAME, DURATION_NS); \
    }

#define PROFILER_CONTINUOUS_MEASUREMENT_START(NAME) \
    if (PROF_ENABLED && (bool)Profiler::ContinuousMeasurementEnabled::NAME) { \
        Profiler::start_continuous_measurement(Profiler::ContinuousMeasurementIndex::NAME); \
    }
#define PROFILER_CONTINUOUS_MEASUREMENT_STOP(NAME) \
    if (PROF_ENABLED && (bool)Profiler::ContinuousMeasurementEnabled::NAME) { \
        Profiler::stop_continuous_measurement(Profiler::ContinuousMeasurementIndex::NAME); \
    }

#define PROFILER_START_STOPWATCH(NAME) \
    if (PROF_ENABLED) NAME.start();
#define PROFILER_STOP_STOPWATCH(NAME) \
    if (PROF_ENABLED) NAME.stop();
#endif // PROFILER_HPP
