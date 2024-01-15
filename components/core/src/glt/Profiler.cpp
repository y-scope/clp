#include "Profiler.hpp"

#include <memory>

using std::unique_ptr;
using std::vector;

namespace glt {
vector<Stopwatch>* Profiler::m_fragmented_measurements = nullptr;
vector<Stopwatch>* Profiler::m_continuous_measurements = nullptr;
}  // namespace glt
