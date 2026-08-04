#ifndef UTILS_PROFILING_STOPWATCH_HPP
#define UTILS_PROFILING_STOPWATCH_HPP

#include <chrono>
#include <cstdint>

namespace utils::profiling {
/**
 * Result of a profiling measurement.
 */
struct Measurement {
    // The number of times the measurement was started/stopped.
    uint32_t call_count{0};
    std::chrono::steady_clock::duration duration{0};
};

/**
 * Simple timer class.
 * Calling `stop()` without `start()` is a no-op. Consecutive `start()` calls before `stop()`
 * are no-ops, with the timer still counting from the first call.
 */
class Stopwatch {
public:
    // Methods
    /**
     * @return The elapsed duration and call count.
     */
    [[nodiscard]] auto get_measurement() const -> Measurement { return m_measurement; }

    /**
     * Marks the beginning of a timed interval. No-op if already running.
     */
    auto start() -> void {
        if (m_running) {
            return;
        }
        m_begin = std::chrono::steady_clock::now();
        m_running = true;
    }

    /**
     * Accumulates the elapsed time since `start()` and increments the call count. No-op if
     * not running.
     */
    auto stop() -> void {
        if (false == m_running) {
            return;
        }
        m_measurement.duration += std::chrono::steady_clock::now() - m_begin;
        ++m_measurement.call_count;
        m_running = false;
    }

private:
    // Data members
    std::chrono::time_point<std::chrono::steady_clock> m_begin;
    Measurement m_measurement;
    bool m_running{false};
};
}  // namespace utils::profiling

#endif  // UTILS_PROFILING_STOPWATCH_HPP
