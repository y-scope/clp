#include "Profiler.hpp"

using std::vector;

namespace clp {
std::unordered_map<std::string, Stopwatch> Profiler::m_runtime_measurements;
vector<Stopwatch>* Profiler::m_compile_time_measurements = nullptr;
bool Profiler::m_initialized = false;
}  // namespace clp
