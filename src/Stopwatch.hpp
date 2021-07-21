#ifndef STOPWATCH_HPP
#define STOPWATCH_HPP

// C++ libraries
#include <chrono>
#include <ctime>
#include <ostream>

class Stopwatch {
public:
    // Constructor
    Stopwatch ();

    // Methods
    void start ();
    void stop ();
    void reset ();

    double get_time_taken_in_seconds ();
    uint64_t get_time_taken_in_nanoseconds () const { return m_time_taken.count(); }

private:
    // Variables
    std::chrono::time_point<std::chrono::high_resolution_clock> m_begin;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_end;
    std::chrono::duration<uint64_t, std::nano> m_time_taken;
};

#endif // STOPWATCH_HPP
