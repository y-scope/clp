#ifndef CLP_STOPWATCH_HPP
#define CLP_STOPWATCH_HPP

#include <chrono>
#include <cstdint>

namespace clp {
class Stopwatch {
public:
    // Constructor
    Stopwatch();

    // Methods
    void start();
    void stop();
    void reset();

    [[nodiscard]] auto get_time_taken_in_seconds() const -> double;

    [[nodiscard]] auto get_call_count() const -> uint32_t;

private:
    // Variables
    std::chrono::time_point<std::chrono::steady_clock> m_begin;
    std::chrono::duration<uint64_t, std::nano> m_time_taken;
    uint32_t m_call_count{0};
};
}  // namespace clp

#endif  // CLP_STOPWATCH_HPP
