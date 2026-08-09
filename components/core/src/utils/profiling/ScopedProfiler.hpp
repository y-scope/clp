#ifndef UTILS_PROFILING_SCOPEDPROFILER_HPP
#define UTILS_PROFILING_SCOPEDPROFILER_HPP

#include <string>
#include <string_view>

#include <utils/profiling/Profiler.hpp>

namespace utils::profiling {
/**
 * RAII wrapper that starts a measurement on construction and stops it on destruction.
 *
 * Intended to be used through the `PROFILE_SCOPE*` macros, but can be used directly.
 * On construction, it computes the full hierarchical name (active prefix + scope name), starts the
 * measurement, and pushes the full name as the new active prefix. On destruction, it pops the
 * prefix and stops the measurement. Therefore, nested `ScopedProfiler`s produce hierarchical names.
 *
 * When profiling is disabled (`CLP_ENABLE_PROFILING == 0`), all method bodies are empty and data
 * member instantiation is negligible (or may be optimized out completely).
 */
class ScopedProfiler {
public:
    // Constructors
    ScopedProfiler() = delete;

    /**
     * Starts a new measurement for `name` that is stopped on destruction.
     * Always computes and stores/owns the full name.
     *
     * @param name The measurement name.
     */
    explicit ScopedProfiler(std::string_view name)
            : m_owned_full_name{Profiler::build_full_name(name)} {
        if constexpr (CLP_ENABLE_PROFILING) {
            m_full_name = m_owned_full_name;
            Profiler::start_measurement(m_full_name);
            Profiler::push_prefix(m_full_name);
        }
    }

    /**
     * Starts a new measurement for `name` that is stopped on destruction.
     * Uses external storage to cache the `name` and `prefix` strings to avoid
     * allocation/recomputation on subsequent invocations (with the same name and scope nesting).
     * On the first invocation, the full name and prefix are computed and stored in the cached
     * arguments.
     *
     * @param name The measurement name.
     * @param cached_full_name The per-call-site cache for the full name. Owned externally.
     * @param cached_prefix The per-call-site cache for the prefix that was active when
     * `cached_full_name` was computed. Owned externally.
     */
    ScopedProfiler(
            std::string_view name,
            std::string& cached_full_name,
            std::string& cached_prefix
    ) {
        if constexpr (CLP_ENABLE_PROFILING) {
            auto const prefix{Profiler::get_active_prefix()};
            if (prefix == cached_prefix) {
                m_full_name = cached_full_name;
            } else {
                cached_prefix = std::string{prefix};
                cached_full_name = Profiler::build_full_name(name);
                m_full_name = cached_full_name;
            }
            Profiler::start_measurement(m_full_name);
            Profiler::push_prefix(m_full_name);
        }
    }

    // Delete copy constructor and assignment operator.
    ScopedProfiler(ScopedProfiler const&) = delete;
    auto operator=(ScopedProfiler const&) -> ScopedProfiler& = delete;

    // Delete move constructor and assignment operator.
    ScopedProfiler(ScopedProfiler&&) = delete;
    auto operator=(ScopedProfiler&&) -> ScopedProfiler& = delete;

    // Destructor
    /**
     * Pops the prefix pushed in the constructor and stops the measurement.
     */
    ~ScopedProfiler() {
        if constexpr (CLP_ENABLE_PROFILING) {
            Profiler::pop_prefix();
            Profiler::stop_measurement(m_full_name);
        }
    }

private:
    // Data members
    // Points to m_owned_name or the externally cached full name.
    std::string_view m_full_name;
    // Owned full name storage.
    std::string m_owned_full_name;
};
}  // namespace utils::profiling

/**
 * `PROFILE_SCOPE` and `PROFILE_SCOPE_DEBUG` create a `ScopedProfiler` for the current scope.
 *
 * Set `CLP_ENABLE_PROFILING=1` to enable `PROFILE_SCOPE`, or `CLP_ENABLE_PROFILING=2` to also
 * enable `PROFILE_SCOPE_DEBUG`. When profiling is disabled, both macros expand to no-ops.
 *
 * `__COUNTER__` ensures each instance is unique, allowing multiple measurements in the same
 * scope. The macros use `static thread_local` cache variables to avoid recomputing the full
 * hierarchical name on repeated invocations at the same nesting level.
 */
#if CLP_ENABLE_PROFILING > 0
   // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define PROFILE_SCOPE_IMPL(counter, name) \
        static thread_local ::std::string _porfile_scope_full_name_##counter; \
        static thread_local ::std::string _porfile_scope_prefix_##counter; \
        ::utils::profiling::ScopedProfiler const _porfile_scope_profiler_##counter { \
            name, _porfile_scope_full_name_##counter, _porfile_scope_prefix_##counter \
        }

    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define PROFILE_SCOPE(name) PROFILE_SCOPE_IMPL(__COUNTER__, name)
#else
   // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define PROFILE_SCOPE(name) (void)0
#endif

#if CLP_ENABLE_PROFILING > 1
   // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define PROFILE_SCOPE_DEBUG(name) PROFILE_SCOPE_IMPL(__COUNTER__, name)
#else
   // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define PROFILE_SCOPE_DEBUG(name) (void)0
#endif

#endif  // UTILS_PROFILING_SCOPEDPROFILER_HPP
