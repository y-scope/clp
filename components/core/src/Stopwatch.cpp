#include "Stopwatch.hpp"
#include <iostream>
#include <utility>
#include <spdlog/spdlog.h>

std::unordered_map<std::string, Stopwatch> Stopwatches::active_watches;

Stopwatch::Stopwatch () : m_time_taken() {
    reset();
}

Stopwatch::Stopwatch (std::string name) : name(std::move(name)), m_time_taken() {
    reset();
}

void Stopwatch::start () {
    m_begin = std::chrono::high_resolution_clock::now();
}

void Stopwatch::stop () {
    m_end = std::chrono::high_resolution_clock::now();
    auto clock_start = std::chrono::high_resolution_clock::now(); 
    auto clock_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<uint64_t, std::nano> clock_time = clock_end - clock_start;
    std::chrono::duration<uint64_t, std::nano> time_taken = m_end - m_begin;
    m_time_taken += time_taken - clock_time;
}

void Stopwatch::reset () {
    m_time_taken = std::chrono::high_resolution_clock::duration::zero();
}

double Stopwatch::get_time_taken_in_seconds () {
    std::chrono::duration<double> time_taken_in_seconds = m_time_taken;
    return time_taken_in_seconds.count();
}

void Stopwatch::print() {
    if (get_time_taken_in_seconds() != 0) {
        SPDLOG_INFO("Stopwatch " + name + " : " + std::to_string(get_time_taken_in_seconds()));
    }
}

static void insert(std::unordered_map<std::string, Stopwatch>& map, const std::string& name) {
    map.insert(std::pair<std::string, Stopwatch>(name, Stopwatch(name)));
}

#ifdef PROFILING

void Stopwatches::init() {
    // LogParser.cpp
    //insert(active_watches, "string_copies_watch");
    //insert(active_watches, "ptr_creation_watch");
    //insert(active_watches, "cast_watch");
    //insert(active_watches, "write_watch");
    //test-ParseWithUserSchema.cpp
    //insert(active_watches, "schema_parser_creation_watch");
    //insert(active_watches, "log_parser_creation_watch");
    //FileCompressor.cpp
    //insert(active_watches, "parse_watch");
    //insert(active_watches, "write_to_file_watch");
}

void Stopwatches::start(std::string& name) {
    std::unordered_map<std::string,Stopwatch>::iterator it = active_watches.find(name);
    if (it != active_watches.end()) {
        it->second.start();
    }
}

void Stopwatches::stop(std::string& name) {
    std::unordered_map<std::string,Stopwatch>::iterator it = active_watches.find(name);
    if (it != active_watches.end()) {
        it->second.stop();
    }
}

void Stopwatches::print() {
    for (auto it : active_watches) {
        it.second.print();
    }
}

#else
void Stopwatches::init() {
}

void Stopwatches::start(std::string& name) {
}

void Stopwatches::stop(std::string& name) {
}

void Stopwatches::print() {
}
#endif