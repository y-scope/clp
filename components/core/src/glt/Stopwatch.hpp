#ifndef GLT_STOPWATCH_HPP
#define GLT_STOPWATCH_HPP

#include <chrono>
#include <ctime>
#include <ostream>

namespace glt {
class Stopwatch {
public:
    // Constructor
    Stopwatch();

    // Methods
    void start();
    void stop();
    void reset();

    double get_time_taken_in_seconds();

private:
    // Variables
    std::chrono::time_point<std::chrono::steady_clock> m_begin;
    std::chrono::duration<uint64_t, std::nano> m_time_taken;
};
}  // namespace glt

#endif  // GLT_STOPWATCH_HPP
