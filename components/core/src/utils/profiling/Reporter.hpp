#ifndef UTILS_PROFILING_REPORTER_HPP
#define UTILS_PROFILING_REPORTER_HPP

#include <cassert>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <variant>

#include <utils/profiling/Profiler.hpp>
#include <utils/profiling/Stopwatch.hpp>

namespace utils::profiling {
/**
 * RAII wrapper that ties together a profiler and sink. All profiler measurements are emit to the
 * sink on destruction and the active profiler is updated.
 *
 * The reporter owns its own `Profiler` instance. On construction, it saves the current thread-local
 * active profiler and prefix, then sets its own profiler and a new hierarchical prefix as active.
 * On destruction, it emits from its own profiler, restores the previous profiler and prefix, and
 * asserts that it is being destroyed on the same thread that created it.
 *
 * When profiling is disabled (`CLP_ENABLE_PROFILING == 0`), all method bodies are guarded by `if
 * constexpr`, so the compiler emits no code. The data members are still instantiated, but their
 * default constructors are effectively zero-cost and this avoids needing a separate emptry stub
 * type.
 *
 * @tparam SinkType The concrete sink type, or `std::variant<...>` of sink types.
 */
template <typename SinkType>
class Reporter {
public:
    // Constructors
    Reporter() = delete;

    /**
     * @param name The reporter name, used to build the hierarchical measurement prefix.
     * @param args Arguments forwarded to the sink's constructor.
     */
    template <typename... Args>
    explicit Reporter(std::string_view name, Args&&... args) : m_sink{std::forward<Args>(args)...} {
        if constexpr (CLP_ENABLE_PROFILING) {
            m_thread_id = std::this_thread::get_id();
            m_prev_profiler = Profiler::get_active_profiler();
            m_prev_prefix = Profiler::get_active_prefix();
            Profiler::set_active_prefix(Profiler::build_full_name(name));
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
    ~Reporter() {
        if constexpr (CLP_ENABLE_PROFILING) {
            assert(m_thread_id == std::this_thread::get_id());
            Profiler::set_active_profiler(m_prev_profiler);
            Profiler::set_active_prefix(std::move(m_prev_prefix));
            m_profiler.for_each_measurement(
                    [this](std::string_view name, Measurement const& measurement) -> void {
                        emit_to_sink(m_sink, name, measurement);
                    }
            );
        }
    }

private:
    // Static methods
    /**
     * Emits a single measurement to the sink. Calls `sink.emit` directly.
     */
    static auto emit_to_sink(auto& sink, std::string_view name, Measurement const& m) -> void {
        sink.emit(name, m);
    }

    /**
     * Emits a single measurement to the active alternative of a `std::variant` sink. Chosen through
     * overload resolution over the generic version.
     */
    template <typename... Ts>
    static auto emit_to_sink(std::variant<Ts...>& sink, std::string_view name, Measurement const& m)
            -> void {
        std::visit([&](auto& s) -> void { s.emit(name, m); }, sink);
    }

    // Data members
    Profiler m_profiler;
    Profiler* m_prev_profiler{nullptr};
    std::string m_prev_prefix;
    SinkType m_sink;
    std::thread::id m_thread_id;
};
}  // namespace utils::profiling

#endif  // UTILS_PROFILING_REPORTER_HPP
