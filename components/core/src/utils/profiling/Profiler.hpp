#ifndef UTILS_PROFILING_PROFILER_HPP
#define UTILS_PROFILING_PROFILER_HPP

#include <string>
#include <string_view>

#if defined(CLP_ENABLE_PROFILING) && CLP_ENABLE_PROFILING > 0
    #include <vector>

    #include <absl/container/flat_hash_map.h>
    #include <spdlog/spdlog.h>
    #include <utils/profiling/Stopwatch.hpp>
#endif

namespace utils::profiling {
#if defined(CLP_ENABLE_PROFILING) && CLP_ENABLE_PROFILING > 0
/**
 * Thread-local registry of named `Stopwatch` measurements.
 *
 * The active profiler records `Stopwatch` measurements. If no profiler is active, all measurement
 * methods are no-ops.
 *
 * Hierarchical names are built as `<scope_path>.<name>`, where the scope path is a thread-local
 * stack of `string_view`s pushed/popped by callers. Each entry must point to stable storage that
 * outlives the corresponding pop.
 */
class Profiler {
public:
    // Static methods
    /**
     * Builds the full measurement name by prepending the active scope path.
     *
     * @param name The local measurement name.
     * @return `"<scope_path>.<name>"` if a scope path is active, or `name` if no scope path is
     * active.
     */
    [[nodiscard]] static auto build_full_name(std::string_view name) -> std::string {
        auto const scope_path{get_active_scope_path()};
        if (scope_path.empty()) {
            return std::string{name};
        }
        std::string result;
        result.reserve(scope_path.size() + 1 + name.size());
        result.append(scope_path);
        result.push_back('.');
        result.append(name);
        return result;
    }

    /**
     * @return The thread-local pointer to the active `Profiler`, or `nullptr` if none is
     * active on the current thread.
     */
    [[nodiscard]] static auto get_active_profiler() -> Profiler* {
        if (m_active_profiler_stack.empty()) {
            return nullptr;
        }
        return m_active_profiler_stack.back();
    }

    /**
     * Pushes a `Profiler` onto the thread-local active profiler stack. The caller must ensure
     * `profiler` outlives the corresponding `pop_active_profiler()` call.
     *
     * @param profiler The profiler to push.
     */
    static auto push_active_profiler(Profiler* profiler) -> void {
        m_active_profiler_stack.push_back(profiler);
    }

    /**
     * Pops the top of the thread-local active profiler stack. No-op if the stack is empty.
     */
    static auto pop_active_profiler() -> void {
        if (not m_active_profiler_stack.empty()) {
            m_active_profiler_stack.pop_back();
        }
    }

    /**
     * @return The thread-local active scope path (top of the scope path stack), or an empty
     * `string_view` if no scope path is active.
     */
    [[nodiscard]] static auto get_active_scope_path() -> std::string_view {
        if (m_scope_path_stack.empty()) {
            return {};
        }
        return m_scope_path_stack.back();
    }

    /**
     * Pushes a scope path onto the thread-local scope path stack. The caller must ensure
     * `scope_path` outlives the corresponding `pop_scope_path()` call.
     *
     * @param scope_path The scope path to push.
     */
    static auto push_scope_path(std::string_view scope_path) -> void {
        m_scope_path_stack.push_back(scope_path);
    }

    /**
     * Pops the top of the thread-local scope path stack. No-op if the stack is empty.
     */
    static auto pop_scope_path() -> void {
        if (not m_scope_path_stack.empty()) {
            m_scope_path_stack.pop_back();
        }
    }

    /**
     * Starts a Stopwatch identified by `full_name`. If it does not yet exist, one is created.
     * If the measurement is already running (re-entrant call), this is a no-op. If no profiler is
     * active on the current thread, this is a no-op.
     *
     * @param full_name The full measurement name.
     */
    static auto start_measurement(std::string_view full_name) -> void {
        auto* const profiler{get_active_profiler()};
        if (nullptr == profiler) {
            return;
        }
        auto const [it, inserted]{profiler->m_stopwatches.try_emplace(std::string{full_name})};
        it->second.start();
    }

    /**
     * Stops the Stopwatch identified by `full_name`. If it does not exist, logs an error and
     * returns. If the measurement exists but is not running, this is a no-op. If no profiler is
     * active on the current thread, this is a no-op.
     *
     * @param full_name The full measurement name.
     */
    static auto stop_measurement(std::string_view full_name) -> void {
        auto* const profiler{get_active_profiler()};
        if (nullptr == profiler) {
            return;
        }
        auto const it{profiler->m_stopwatches.find(full_name)};
        if (it == profiler->m_stopwatches.end()) {
            SPDLOG_ERROR("Attempt to stop non-existent runtime measurement: {}", full_name);
            return;
        }
        it->second.stop();
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
        for (auto const& [name, stopwatch] : m_stopwatches) {
            auto const measurement{stopwatch.get_measurement()};
            if (0 == measurement.call_count) {
                continue;
            }
            callback(std::string_view{name}, measurement);
        }
        m_stopwatches.clear();
    }

private:
    // Static data members
    static inline thread_local std::vector<Profiler*> m_active_profiler_stack;
    static inline thread_local std::vector<std::string_view> m_scope_path_stack;

    // Data members
    absl::flat_hash_map<std::string, Stopwatch> m_stopwatches;
};
#else
/**
 * Stub used when profiling is disabled (`CLP_ENABLE_PROFILING == 0`).
 */
class Profiler {
public:
    // Static methods
    [[nodiscard]] static auto build_full_name(std::string_view name) -> std::string { return {}; }

    [[nodiscard]] static auto get_active_profiler() -> Profiler* { return nullptr; }

    static auto push_active_profiler(Profiler* profiler) -> void {}

    static auto pop_active_profiler() -> void {}

    [[nodiscard]] static auto get_active_scope_path() -> std::string_view { return {}; }

    static auto push_scope_path(std::string_view scope_path) -> void {}

    static auto pop_scope_path() -> void {}

    static auto start_measurement(std::string_view full_name) -> void {}

    static auto stop_measurement(std::string_view full_name) -> void {}

    // Methods
    template <typename Callback>
    auto for_each_measurement(Callback callback) -> void {}
};
#endif  // defined(CLP_ENABLE_PROFILING) && CLP_ENABLE_PROFILING > 0
}  // namespace utils::profiling

#endif  // UTILS_PROFILING_PROFILER_HPP
