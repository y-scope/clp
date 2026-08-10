#ifndef UTILS_PROFILING_REPORTER_HPP
#define UTILS_PROFILING_REPORTER_HPP

#include <cassert>
#include <chrono>
#include <concepts>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <spdlog/spdlog.h>
#include <utils/profiling/Profiler.hpp>
#include <utils/profiling/Stopwatch.hpp>

namespace utils::profiling {
/**
 * Concept constraining the emit callback type for `Reporter`.
 */
template <typename F>
concept MeasurementEmitter = std::invocable<F&, std::string_view, Measurement>;

/**
 * Emit callback that writes profiler measurements to SPDLOG in milliseconds.
 */
struct SpdlogEmitter {
    auto operator()(std::string_view name, Measurement measurement) const -> void {
        SPDLOG_INFO(
                "{}: {} millisecs ({} calls)",
                name,
                std::chrono::duration_cast<std::chrono::milliseconds>(measurement.duration).count(),
                measurement.call_count
        );
    }
};

/**
 * RAII wrapper that collects profiler measurements and emits them to a callback on destruction.
 *
 * On construction, the reporter registers its `Profiler` as the thread-local active profiler and
 * pushes its name as the active prefix, so that all `ScopedProfiler` instances created within its
 * scope produce hierarchical measurement names. On destruction, it restores the previous active
 * profiler and prefix, then emits all collected measurements.
 *
 * A reporter must be created and destroyed on the same thread. For multi-threaded profiling, each
 * worker thread should create its own `Reporter`.
 *
 * When profiling is disabled (`CLP_ENABLE_PROFILING == 0`), all method bodies are empty and data
 * member instantiation is negligible (or may be optimized out completely).
 *
 * @tparam EmitCallback The callback type, constrained by `MeasurementEmitter`.
 */
template <typename EmitCallback>
requires MeasurementEmitter<EmitCallback>
class Reporter {
public:
    // Constructors
    Reporter() = delete;

    /**
     * Sets its own `Profiler` as the active `Profiler`, stores the previous active `Profiler`, and
     * pushes its name onto the prefix.
     *
     * @param name The reporter name, used to build the hierarchical measurement prefix.
     * @param emit Callback invoked once per measurement on destruction.
     */
    explicit Reporter(std::string_view name, EmitCallback emit) : m_emit{std::move(emit)} {
        if constexpr (CLP_ENABLE_PROFILING) {
            m_thread_id = std::this_thread::get_id();
            m_prev_profiler = Profiler::get_active_profiler();
            m_full_name = Profiler::build_full_name(name);
            Profiler::push_prefix(m_full_name);
            Profiler::set_active_profiler(&m_profiler);
        }
    }

    // Delete copy constructor and assignment operator.
    Reporter(Reporter const&) = delete;
    auto operator=(Reporter const&) -> Reporter& = delete;

    // Delete move constructor and assignment operator.
    Reporter(Reporter&&) = delete;
    auto operator=(Reporter&&) -> Reporter& = delete;

    // Destructor
    /**
     * Sets the active `Profiler` to the previous one stored on construction, pops the prefix, and
     * emits the measurements.
     */
    ~Reporter() {
        if constexpr (CLP_ENABLE_PROFILING) {
            assert(m_thread_id == std::this_thread::get_id());
            Profiler::set_active_profiler(m_prev_profiler);
            Profiler::pop_prefix();
            m_profiler.for_each_measurement(
                    [this](std::string_view name, Measurement const& measurement) -> void {
                        m_emit(name, measurement);
                    }
            );
        }
    }

private:
    // Data members
    Profiler m_profiler;
    Profiler* m_prev_profiler{nullptr};
    std::string m_full_name;
    EmitCallback m_emit;
    std::thread::id m_thread_id;
};
}  // namespace utils::profiling

#endif  // UTILS_PROFILING_REPORTER_HPP
