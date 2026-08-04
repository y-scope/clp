#ifndef UTILS_PROFILING_PROFILER_HPP
#define UTILS_PROFILING_PROFILER_HPP

#ifndef CLP_ENABLE_PROFILING
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define CLP_ENABLE_PROFILING 0
#endif

#include <string>
#include <string_view>

#include <absl/container/flat_hash_map.h>
#include <spdlog/spdlog.h>
#include <utils/profiling/Stopwatch.hpp>

namespace utils::profiling {
/**
 * Thread-local management of named `Stopwatch` instances that is enabled at compile time.  If
 * profiling is disabled (`CLP_ENABLE_PROFILING == 0`) the functions are empty.
 *
 * A measurement can be taken over a single continuous operation, or called multiple times to
 * accumulate measurements into a total run time.  `Reporter`s own `Profiler`s where the
 * thread-local active `Profiler` records new measurements.  The thread-local active profiler is
 * saved/restored on `Reporter`construction/destruction, enabling nesting.
 * The hierarchical name is built as `<parent-prefix>.<reporter-name>.<scope-name>`. Each
 * nested `Reporter` contributes its name to the prefix.
 *
 * Note that for thread-safety a `Reporter` must be created and destroyed on the same thread.
 * Each thread's measurements are isolated via thread-local
 * storage. For multi-threaded profiling, each worker thread should create its own
 * `Reporter`.
 *
 * When no `Reporter` is active, `PROFILE_SCOPE` and all measurement methods are no-ops.
 */
class Profiler {
public:
    // Static methods
    /**
     * Builds the full measurement name by prepending the active prefix.
     *
     * @param name The local measurement name.
     * @return `"<prefix>.<name>"` if a prefix is active, or `name` if no prefix is active.
     */
    [[nodiscard]] static auto build_full_name(std::string_view name) -> std::string;

    /**
     * @return The thread-local pointer to the active `Profiler`, or `nullptr` if no
     * `Reporter` is active on the current thread.
     */
    [[nodiscard]] static auto get_active_profiler() -> Profiler*;

    /**
     * Sets the thread-local pointer to the active `Profiler`.
     *
     * @param profiler The profiler to set as active, or `nullptr` to deactivate.
     */
    static auto set_active_profiler(Profiler* profiler) -> void;

    /**
     * @return The thread-local active name prefix.
     */
    [[nodiscard]] static auto get_active_prefix() -> std::string_view;

    /**
     * Sets the thread-local active name prefix.
     *
     * @param prefix The prefix to set as active.
     */
    static auto set_active_prefix(std::string prefix) -> void;

    /**
     * Starts a Stopwatch identified by `name`. If it does not yet exist, one is created. If the
     * measurement is already running (re-entrant call), this is a no-op.
     * If no `Reporter` is active on the current thread, this is a no-op.
     *
     * @param name The measurement name (will be prefixed with the active reporter's prefix).
     */
    static auto start_measurement(std::string_view name) -> void {
        if constexpr (CLP_ENABLE_PROFILING) {
            auto* const profiler{get_active_profiler()};
            if (nullptr == profiler) {
                return;
            }
            auto const full_name{build_full_name(name)};
            auto const [it, inserted]{profiler->m_map.try_emplace(full_name)};
            it->second.start();
        }
    }

    /**
     * Stops the Stopwatch identified by `name`. If it does not exist, logs an error and returns. If
     * the measurement exists but is not running, this is a no-op.
     * If no `Reporter` is active on the current thread, this is a no-op.
     *
     * @param name The measurement name (will be prefixed with the active reporter's prefix).
     */
    static auto stop_measurement(std::string_view name) -> void {
        if constexpr (CLP_ENABLE_PROFILING) {
            auto* const profiler{get_active_profiler()};
            if (nullptr == profiler) {
                return;
            }
            auto const full_name{build_full_name(name)};
            auto const it{profiler->m_map.find(full_name)};
            if (it == profiler->m_map.end()) {
                SPDLOG_ERROR("Attempt to stop non-existent runtime measurement: {}", full_name);
                return;
            }
            it->second.stop();
        }
    }

    // Methods
    /**
     * Calls `callback` for each measurement with `call_count > 0`, then clears all
     * measurements. Subsequent calls will yield no results unless new measurements are
     * added after this call.
     *
     * @param callback A callable taking `(std::string_view name, Measurement)`.
     */
    template <typename Callback>
    auto for_each_measurement(Callback callback) -> void {
        if constexpr (CLP_ENABLE_PROFILING) {
            for (auto const& [name, stopwatch] : m_map) {
                auto const measurement{stopwatch.get_measurement()};
                if (0 == measurement.call_count) {
                    continue;
                }
                callback(std::string_view{name}, measurement);
            }
            m_map.clear();
        }
    }

private:
    // Static data members
    static thread_local Profiler* m_active_profiler;
    static thread_local std::string m_active_prefix;

    // Data members
    absl::flat_hash_map<std::string, Stopwatch> m_map;
};
}  // namespace utils::profiling

#endif  // UTILS_PROFILING_PROFILER_HPP
