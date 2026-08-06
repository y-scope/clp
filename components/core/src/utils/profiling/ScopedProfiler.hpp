#ifndef UTILS_PROFILING_SCOPEDPROFILER_HPP
#define UTILS_PROFILING_SCOPEDPROFILER_HPP

#include <string>
#include <string_view>

#include <utils/profiling/Profiler.hpp>

namespace utils::profiling {
/**
 * RAII wrapper that starts a runtime measurement on construction and stops it on destruction.
 *
 * When profiling is disabled (`CLP_ENABLE_PROFILING == 0`), all method bodies are guarded by
 * `if constexpr`, so the compiler emits no code for the profiling logic. The `std::string`
 * member is still instantiated but uses SSO (no heap allocation for short strings).
 */
class ScopedProfiler {
public:
    // Constructors
    ScopedProfiler() = delete;

    /**
     * @param name The measurement name to start on construction and stop on destruction.
     */
    explicit ScopedProfiler(std::string_view name) : m_name{name} {
        if constexpr (CLP_ENABLE_PROFILING) {
            Profiler::start_measurement(m_name);
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
     * Stops the measurement started in the constructor.
     */
    ~ScopedProfiler() {
        if constexpr (CLP_ENABLE_PROFILING) {
            Profiler::stop_measurement(m_name);
        }
    }

private:
    // Data members
    std::string m_name;
};
}  // namespace utils::profiling

/**
 * When CLP_ENABLE_PROFILING is 0, both macros expand to no-ops.
 * Set CLP_ENABLE_PROFILING=1 at the CMake target level to enable PROFILE_SCOPE, or
 * CLP_ENABLE_PROFILING=2 to also enable PROFILE_SCOPE_DEBUG.
 * __COUNTER__ is used to ensure each ScopedProfiler instance is unique and doesn't cause
 * redefinition errors allowing you to start multiple measurements from different locations in the
 * same scope.
 */
#if CLP_ENABLE_PROFILING > 0
   // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define PROFILE_SCOPE(name) \
        ::utils::profiling::ScopedProfiler const _prof_scoped_##__COUNTER__ { \
            name \
        }
#else
   // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define PROFILE_SCOPE(name) (void)0
#endif

#if CLP_ENABLE_PROFILING > 1
   // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define PROFILE_SCOPE_DEBUG(name) \
        ::utils::profiling::ScopedProfiler const _prof_scoped_dbg_##__COUNTER__ { \
            name \
        }
#else
   // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define PROFILE_SCOPE_DEBUG(name) (void)0
#endif

#endif  // UTILS_PROFILING_SCOPEDPROFILER_HPP
