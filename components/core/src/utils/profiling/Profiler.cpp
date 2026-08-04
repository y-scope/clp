#include <string>
#include <string_view>
#include <utility>

#include <utils/profiling/Profiler.hpp>

namespace utils::profiling {
thread_local Profiler* Profiler::m_active_profiler{nullptr};
thread_local std::string Profiler::m_active_prefix;

auto Profiler::get_active_profiler() -> Profiler* {
    return m_active_profiler;
}

auto Profiler::set_active_profiler(Profiler* profiler) -> void {
    m_active_profiler = profiler;
}

auto Profiler::get_active_prefix() -> std::string_view {
    return m_active_prefix;
}

auto Profiler::set_active_prefix(std::string prefix) -> void {
    m_active_prefix = std::move(prefix);
}

auto Profiler::build_full_name(std::string_view name) -> std::string {
    if (m_active_prefix.empty()) {
        return std::string{name};
    }
    return std::string{m_active_prefix} + "." + std::string{name};
}
}  // namespace utils::profiling
