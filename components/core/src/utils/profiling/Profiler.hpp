#ifndef UTILS_PROFILING_PROFILER_HPP
#define UTILS_PROFILING_PROFILER_HPP

#ifndef CLP_ENABLE_PROFILING
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define CLP_ENABLE_PROFILING 0
#endif

#include <string>
#include <string_view>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <spdlog/spdlog.h>
#include <utils/profiling/Stopwatch.hpp>

namespace utils::profiling {
/**
 * Thread-local registry of named `Stopwatch` measurements. When profiling is disabled
 * (`CLP_ENABLE_PROFILING == 0`), all methods are empty.
 *
 * The thread-local active profiler records `Stopwatch` measurements. If no profiler is active, all
 * measurement methods are no-ops.
 *
 * Hierarchical names are built as `<prefix>.<name>`, where the prefix is a thread-local stack of
 * `string_view`s pushed/popped by callers. Each entry must point to stable storage that outlives
 * the corresponding pop.
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
     * @return The thread-local pointer to the active `Profiler`, or `nullptr` if none is
     * active on the current thread.
     */
    [[nodiscard]] static auto get_active_profiler() -> Profiler*;

    /**
     * Sets the thread-local pointer to the active `Profiler`.
     *
     * @param profiler The profiler to set as active, or `nullptr` to deactivate.
     */
    static auto set_active_profiler(Profiler* profiler) -> void;

    /**
     * @return The thread-local active name prefix (top of the prefix stack), or an empty
     * `string_view` if no prefix is active.
     */
    [[nodiscard]] static auto get_active_prefix() -> std::string_view;

    /**
     * Pushes a name prefix onto the thread-local prefix stack. The caller must ensure `prefix`
     * outlives the corresponding `pop_prefix()` call.
     *
     * @param prefix The prefix to push.
     */
    static auto push_prefix(std::string_view prefix) -> void;

    /**
     * Pops the top of the thread-local prefix stack. No-op if the stack is empty.
     */
    static auto pop_prefix() -> void;

    /**
     * Starts a Stopwatch identified by `full_name`. If it does not yet exist, one is created.
     * If the measurement is already running (re-entrant call), this is a no-op.
     * If no profiler is active on the current thread, this is a no-op.
     *
     * @param full_name The full measurement name.
     */
    static auto start_measurement(std::string_view full_name) -> void {
        if constexpr (CLP_ENABLE_PROFILING) {
            auto* const profiler{get_active_profiler()};
            if (nullptr == profiler) {
                return;
            }
            auto const [it, inserted]{profiler->m_map.try_emplace(std::string{full_name})};
            it->second.start();
        }
    }

    /**
     * Stops the Stopwatch identified by `full_name`. If it does not exist, logs an error and
     * returns. If the measurement exists but is not running, this is a no-op. If no profiler is
     * active on the current thread, this is a no-op.
     *
     * @param full_name The full measurement name.
     */
    static auto stop_measurement(std::string_view full_name) -> void {
        if constexpr (CLP_ENABLE_PROFILING) {
            auto* const profiler{get_active_profiler()};
            if (nullptr == profiler) {
                return;
            }
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
     * Calls `callback` for each measurement with `call_count > 0`, then clears all measurements.
     * Subsequent calls will yield no results unless new measurements are added after this call.
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
    static thread_local std::vector<std::string_view> m_prefix_stack;

    // Data members
    absl::flat_hash_map<std::string, Stopwatch> m_map;
};
}  // namespace utils::profiling

#endif  // UTILS_PROFILING_PROFILER_HPP
