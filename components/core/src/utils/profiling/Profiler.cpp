#if defined(CLP_ENABLE_PROFILING) && CLP_ENABLE_PROFILING > 0
    #include <string>
    #include <string_view>
    #include <vector>

    #include <utils/profiling/Profiler.hpp>
#endif

#if defined(CLP_ENABLE_PROFILING) && CLP_ENABLE_PROFILING > 0
namespace utils::profiling {
thread_local std::vector<Profiler*> Profiler::m_active_profiler_stack;
thread_local std::vector<std::string_view> Profiler::m_scope_path_stack;

auto Profiler::get_active_profiler() -> Profiler* {
    if (m_active_profiler_stack.empty()) {
        return nullptr;
    }
    return m_active_profiler_stack.back();
}

auto Profiler::push_active_profiler(Profiler* profiler) -> void {
    m_active_profiler_stack.push_back(profiler);
}

auto Profiler::pop_active_profiler() -> void {
    if (not m_active_profiler_stack.empty()) {
        m_active_profiler_stack.pop_back();
    }
}

auto Profiler::get_active_scope_path() -> std::string_view {
    if (m_scope_path_stack.empty()) {
        return {};
    }
    return m_scope_path_stack.back();
}

auto Profiler::push_scope_path(std::string_view scope_path) -> void {
    m_scope_path_stack.push_back(scope_path);
}

auto Profiler::pop_scope_path() -> void {
    if (not m_scope_path_stack.empty()) {
        m_scope_path_stack.pop_back();
    }
}

auto Profiler::build_full_name(std::string_view name) -> std::string {
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
}  // namespace utils::profiling

#endif  // defined(CLP_ENABLE_PROFILING) && CLP_ENABLE_PROFILING > 0
