#include "Profiler.hpp"

using std::atomic_uint64_t;
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::nano;
using std::unique_ptr;
using std::vector;

unique_ptr<vector<atomic_uint64_t>> Profiler::m_fragmented_measurement_begin_timestamps_ns;
unique_ptr<vector<atomic_uint64_t>> Profiler::m_fragmented_measurement_durations_ns;
unique_ptr<vector<atomic_uint64_t>> Profiler::m_continuous_measurement_begin_timestamps_ns;
unique_ptr<vector<atomic_uint64_t>> Profiler::m_continuous_measurement_durations_ns;

void Profiler::init () {
    m_fragmented_measurement_begin_timestamps_ns = std::make_unique<vector<atomic_uint64_t>>((size_t) FragmentedMeasurementIndex::Length);
    m_fragmented_measurement_durations_ns = std::make_unique<vector<atomic_uint64_t>>((size_t) FragmentedMeasurementIndex::Length);
    for (size_t i = 0; i < m_fragmented_measurement_durations_ns->size(); ++i) {
        (*m_fragmented_measurement_durations_ns)[i] = 0;
    }

    m_continuous_measurement_begin_timestamps_ns = std::make_unique<vector<atomic_uint64_t>>((size_t) ContinuousMeasurementIndex::Length);
    m_continuous_measurement_durations_ns = std::make_unique<vector<atomic_uint64_t>>((size_t) ContinuousMeasurementIndex::Length);
    for (size_t i = 0; i < m_continuous_measurement_durations_ns->size(); ++i) {
        (*m_continuous_measurement_durations_ns)[i] = 0;
    }
}

void Profiler::start_measurement (unique_ptr<vector<atomic_uint64_t>>& begin_timestamps_in_nanoseconds, size_t index) {
    (*begin_timestamps_in_nanoseconds)[index] = duration_cast<duration<uint64_t, nano>>(high_resolution_clock::now().time_since_epoch()).count();
}

void Profiler::stop_measurement (unique_ptr<vector<atomic_uint64_t>>& begin_timestamps_in_nanoseconds,
                                 unique_ptr<vector<atomic_uint64_t>>& durations_in_nanoseconds, size_t index)
{
    auto begin_timestamp = (*begin_timestamps_in_nanoseconds)[index].load();
    uint64_t end_timestamp = duration_cast<duration<uint64_t, nano>>(high_resolution_clock::now().time_since_epoch()).count();
    if (end_timestamp >= begin_timestamp) {
        (*durations_in_nanoseconds)[index] += end_timestamp - begin_timestamp;
    } else {
        (*durations_in_nanoseconds)[index] += (UINT64_MAX - begin_timestamp) + end_timestamp;
    }
}
