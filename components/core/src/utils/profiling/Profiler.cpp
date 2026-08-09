#include <string>
#include <string_view>
#include <vector>

#include <utils/profiling/Profiler.hpp>

namespace utils::profiling {
thread_local Profiler* Profiler::m_active_profiler{nullptr};
thread_local std::vector<std::string_view> Profiler::m_prefix_stack;

auto Profiler::get_active_profiler() -> Profiler* {
    return m_active_profiler;
}

auto Profiler::set_active_profiler(Profiler* profiler) -> void {
    m_active_profiler = profiler;
}

auto Profiler::get_active_prefix() -> std::string_view {
    if (m_prefix_stack.empty()) {
        return {};
    }
    return m_prefix_stack.back();
}

auto Profiler::push_prefix(std::string_view prefix) -> void {
    m_prefix_stack.push_back(prefix);
}

auto Profiler::pop_prefix() -> void {
    if (not m_prefix_stack.empty()) {
        m_prefix_stack.pop_back();
    }
}

auto Profiler::build_full_name(std::string_view name) -> std::string {
    auto const prefix{get_active_prefix()};
    if (prefix.empty()) {
        return std::string{name};
    }
    std::string result;
    result.reserve(prefix.size() + 1 + name.size());
    result.append(prefix);
    result.push_back('.');
    result.append(name);
    return result;
}
}  // namespace utils::profiling
