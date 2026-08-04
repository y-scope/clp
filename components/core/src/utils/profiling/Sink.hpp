#ifndef UTILS_PROFILING_SINK_HPP
#define UTILS_PROFILING_SINK_HPP

#include <string_view>

#include <utils/profiling/Stopwatch.hpp>

namespace utils::profiling {
/**
 * Abstract sink interface for profiler measurements. Concrete sinks deliver measurements to a
 * backend (e.g., SPDLOG, OpenTelemetry span attributes).
 */
class Sink {
public:
    // Constructors
    Sink() = default;

    // Default copy constructor and assignment operator.
    Sink(Sink const&) = default;
    auto operator=(Sink const&) -> Sink& = default;

    // Default move constructor and assignment operator.
    Sink(Sink&&) = default;
    auto operator=(Sink&&) -> Sink& = default;

    // Destructor
    virtual ~Sink() = default;

    // Methods
    /**
     * Delivers a single measurement to the backend.
     * @param name The measurement name.
     * @param measurement
     */
    virtual auto emit(std::string_view name, Measurement measurement) -> void = 0;
};

/**
 * Sink that writes profiler measurements to SPDLOG.
 */
class SpdlogSink : public Sink {
public:
    // Methods implementing Sink
    /**
     * Writes the measurement as an info-level SPDLOG line.
     * @param name The measurement name.
     * @param measurement
     */
    auto emit(std::string_view name, Measurement measurement) -> void override;
};
}  // namespace utils::profiling

#endif  // UTILS_PROFILING_SINK_HPP
