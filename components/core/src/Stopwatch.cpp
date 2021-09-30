#include "Stopwatch.hpp"

Stopwatch::Stopwatch () {
    reset();
}

void Stopwatch::start () {
    m_begin = std::chrono::high_resolution_clock::now();
}

void Stopwatch::stop () {
    m_end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<uint64_t, std::nano> time_taken = m_end - m_begin;
    m_time_taken += time_taken;
}

void Stopwatch::reset () {
    m_time_taken = std::chrono::high_resolution_clock::duration::zero();
}

double Stopwatch::get_time_taken_in_seconds () {
    std::chrono::duration<double> time_taken_in_seconds = m_time_taken;
    return time_taken_in_seconds.count();
}
