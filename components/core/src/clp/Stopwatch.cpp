#include "Stopwatch.hpp"

namespace clp {
Stopwatch::Stopwatch() {
    reset();
}

void Stopwatch::start() {
    m_begin = std::chrono::steady_clock::now();
}

void Stopwatch::stop() {
    auto end = std::chrono::steady_clock::now();

    auto time_taken = end - m_begin;
    m_time_taken += time_taken;
}

void Stopwatch::reset() {
    m_time_taken = std::chrono::steady_clock::duration::zero();
}

double Stopwatch::get_time_taken_in_seconds() {
    std::chrono::duration<double> time_taken_in_seconds = m_time_taken;
    return time_taken_in_seconds.count();
}
}  // namespace clp
