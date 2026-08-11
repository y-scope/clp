#ifndef UTILS_PROFILING_REPORTER_HPP
#define UTILS_PROFILING_REPORTER_HPP

#include <chrono>
#include <string_view>
#include <utility>

#include <spdlog/spdlog.h>
#include <utils/profiling/Stopwatch.hpp>

#if defined(CLP_ENABLE_PROFILING) && CLP_ENABLE_PROFILING > 0
    #include <concepts>
    #include <string>

    #include <utils/profiling/Profiler.hpp>
#endif

namespace utils::profiling {
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

#if defined(CLP_ENABLE_PROFILING) && CLP_ENABLE_PROFILING > 0
/**
 * Concept constraining the emit callback type for `Reporter`.
 */
template <typename F>
concept MeasurementEmitter = std::invocable<F&, std::string_view, Measurement>;

/**
 * RAII wrapper that collects profiler measurements and emits them to a callback on destruction.
 *
 * On construction, the reporter pushes its `Profiler` onto the thread-local active profiler stack
 * and pushes its name onto the thread-local scope path stack, so that any profiling using
 * `Profiler` collected within the `Reporter`'s scope produce hierarchical measurement names. On
 * destruction, it pops both stacks and emits all collected measurements.
 *
 * For multi-threaded profiling, each worker thread should create its own `Reporter`.
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
     * Pushes its own `Profiler` onto the active profiler stack and pushes its name onto the scope
     * path stack. Any new profiler measurements will be recorded under `m_profiler`.
     *
     * @param name The reporter name, used to build the hierarchical measurement scope path.
     * @param emit Callback invoked once per measurement on destruction.
     */
    explicit Reporter(std::string_view name, EmitCallback emit) : m_emit{std::move(emit)} {
        m_full_name = Profiler::build_full_name(name);
        Profiler::push_scope_path(m_full_name);
        Profiler::push_active_profiler(&m_profiler);
    }

    // Delete copy constructor and assignment operator
    Reporter(Reporter const&) = delete;
    auto operator=(Reporter const&) -> Reporter& = delete;

    // Delete move constructor and assignment operator
    Reporter(Reporter&&) = delete;
    auto operator=(Reporter&&) -> Reporter& = delete;

    // Destructor
    /**
     * Pops the active profiler and scope path stacks, then emits the profiler's measurements.
     */
    ~Reporter() {
        Profiler::pop_active_profiler();
        Profiler::pop_scope_path();
        m_profiler.for_each_measurement(
                [this](std::string_view name, Measurement const& measurement) -> void {
                    m_emit(name, measurement);
                }
        );
    }

private:
    // Data members
    Profiler m_profiler;
    std::string m_full_name;
    EmitCallback m_emit;
};
#else
/**
 * Stub used when profiling is disabled (`CLP_ENABLE_PROFILING == 0`).
 */
class Reporter {
public:
    // Constructors
    Reporter() = delete;

    template <typename EmitCallback>
    explicit Reporter(std::string_view name, EmitCallback emit) {}

    // Delete copy constructor and assignment operator
    Reporter(Reporter const&) = delete;
    auto operator=(Reporter const&) -> Reporter& = delete;

    // Delete move constructor and assignment operator
    Reporter(Reporter&&) = delete;
    auto operator=(Reporter&&) -> Reporter& = delete;

    // Destructor
    ~Reporter() = default;
};
#endif  // defined(CLP_ENABLE_PROFILING) && CLP_ENABLE_PROFILING > 0
}  // namespace utils::profiling

#endif  // UTILS_PROFILING_REPORTER_HPP
