#ifndef UTILS_PROFILING_SCOPEDPROFILER_HPP
#define UTILS_PROFILING_SCOPEDPROFILER_HPP

#if defined(CLP_ENABLE_PROFILING) && CLP_ENABLE_PROFILING > 0
    #include <string>
    #include <string_view>

    #include <utils/profiling/Profiler.hpp>
#endif

#if defined(CLP_ENABLE_PROFILING) && CLP_ENABLE_PROFILING > 0
namespace utils::profiling {
/**
 * RAII wrapper that starts a measurement on construction and stops it on destruction.
 *
 * Should only be used through the `PROFILE_SCOPE*` macros.
 * On construction, it computes the full hierarchical name (<active scope path>.<scope name>),
 * starts the measurement, and pushes the full name as the new active scope path. On destruction, it
 * pops the scope path and stops the measurement.
 *
 * Can be used directly in unit testing as CLP_ENABLE_PROFILING is always enabled, but any direct
 * usage with profiling disabled will not compile.
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
    explicit ScopedProfiler(std::string_view name) : m_full_name{Profiler::build_full_name(name)} {
        Profiler::start_measurement(m_full_name);
        Profiler::push_scope_path(m_full_name);
    }

    /**
     * Starts a new measurement named <scope_path>.<name> that is stopped on destruction.
     * Uses external storage to cache the full name and scope path to avoid
     * allocation/recomputation on subsequent invocations (with the same name and scope path).
     * On the first invocation, the full name and scope path are computed and stored in the cached
     * arguments.
     *
     * @param name The measurement name.
     * @param cached_full_name Per-call-site cache for the full name. Owned externally.
     * @param cached_scope_path Per-call-site cache for the scope path that was active when
     * `cached_full_name` was computed. Owned externally.
     * @param cached_name Per-call-site cache for the name used to compute `cached_full_name`.
     * Owned externally.
     */
    ScopedProfiler(
            std::string_view name,
            std::string& cached_full_name,
            std::string& cached_scope_path,
            std::string& cached_name
    ) {
        auto const scope_path{Profiler::get_active_scope_path()};
        if (scope_path == cached_scope_path && name == cached_name) {
            m_full_name = cached_full_name;
        } else {
            cached_scope_path = std::string{scope_path};
            cached_name = std::string{name};
            cached_full_name = Profiler::build_full_name(name);
            m_full_name = cached_full_name;
        }
        Profiler::start_measurement(m_full_name);
        Profiler::push_scope_path(m_full_name);
    }

    // Delete copy constructor and assignment operator
    ScopedProfiler(ScopedProfiler const&) = delete;
    auto operator=(ScopedProfiler const&) -> ScopedProfiler& = delete;

    // Delete move constructor and assignment operator
    ScopedProfiler(ScopedProfiler&&) = delete;
    auto operator=(ScopedProfiler&&) -> ScopedProfiler& = delete;

    // Destructor
    /**
     * Pops the scope path pushed in the constructor and stops the measurement.
     */
    ~ScopedProfiler() {
        Profiler::pop_scope_path();
        Profiler::stop_measurement(m_full_name);
    }

private:
    // Data members
    std::string m_full_name;
};
}  // namespace utils::profiling
#endif  // defined(CLP_ENABLE_PROFILING) && CLP_ENABLE_PROFILING > 0

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
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#if defined(CLP_ENABLE_PROFILING) && CLP_ENABLE_PROFILING > 0
    #define PROFILE_SCOPE_IMPL(counter, name) \
        static thread_local ::std::string _prof_scope_full_name_##counter; \
        static thread_local ::std::string _prof_scope_path_##counter; \
        static thread_local ::std::string _prof_scope_name_##counter; \
        ::utils::profiling::ScopedProfiler const _prof_scope_profiler_##counter { \
            name, _prof_scope_full_name_##counter, _prof_scope_path_##counter, \
                    _prof_scope_name_##counter \
        }

    #define PROFILE_SCOPE_EXPAND(counter, name) PROFILE_SCOPE_IMPL(counter, name)

    #define PROFILE_SCOPE(name) PROFILE_SCOPE_EXPAND(__COUNTER__, name)
#else
    #define PROFILE_SCOPE(name) (void)0
#endif

#if defined(CLP_ENABLE_PROFILING) && CLP_ENABLE_PROFILING > 1
    #define PROFILE_SCOPE_DEBUG(name) PROFILE_SCOPE_EXPAND(__COUNTER__, name)
#else
    #define PROFILE_SCOPE_DEBUG(name) (void)0
#endif
// NOLINTEND(cppcoreguidelines-macro-usage)

#endif  // UTILS_PROFILING_SCOPEDPROFILER_HPP
