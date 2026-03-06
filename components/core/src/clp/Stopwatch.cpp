#include "Stopwatch.hpp"

#include <chrono>
#include <cstdint>

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

    m_call_count++;
}

void Stopwatch::reset() {
    m_time_taken = std::chrono::steady_clock::duration::zero();
}

auto Stopwatch::get_time_taken_in_seconds() const -> double {
    std::chrono::duration<double> time_taken_in_seconds = m_time_taken;
    return time_taken_in_seconds.count();
}

auto Stopwatch::get_call_count() const -> uint32_t {
    return m_call_count;
}
}  // namespace clp
