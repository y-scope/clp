#ifndef STOPWATCH_HPP
#define STOPWATCH_HPP
// #define PROFILING

// C++ libraries
#include <chrono>
#include <ctime>
#include <ostream>
#include <unordered_map>

class Stopwatch {
public:
    // Constructor
    Stopwatch ();
    explicit Stopwatch (std::string name);
    
    // Methods
    void start ();
    void stop ();
    void reset ();
    void print ();

    double get_time_taken_in_seconds ();
    [[nodiscard]] uint64_t get_time_taken_in_nanoseconds () const { return m_time_taken.count(); }
    
private:
    // Variables
    std::chrono::time_point<std::chrono::high_resolution_clock> m_begin;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_end;
    std::chrono::duration<uint64_t, std::nano> m_time_taken;
    std::string name;
};

class Stopwatches{
public:
    static std::unordered_map<std::string, Stopwatch> active_watches;
    Stopwatches() =delete;
    static void init();
    static void start(std::string&);
    static void stop(std::string&);
    static void print();
};

#endif // STOPWATCH_HPP
