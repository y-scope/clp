#include "Profiler.hpp"

// C++ standard libraries
#include <memory>

using std::unique_ptr;
using std::vector;

vector<Stopwatch>* Profiler::m_fragmented_measurements = nullptr;
vector<Stopwatch>* Profiler::m_continuous_measurements = nullptr;
