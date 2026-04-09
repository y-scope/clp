#ifndef TESTS_LOGGENERATOR_HPP
#define TESTS_LOGGENERATOR_HPP

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "LogType.hpp"

class LogGenerator {
public:
    explicit LogGenerator(uint32_t seed = 101);

    auto generate_logs(size_t count) -> std::vector<std::string>;

private:
    auto generate_value(VarType type) -> std::string;

    auto generate_string() -> std::string;

    std::mt19937 m_rng;
    std::vector<LogType> m_log_types;
};

#endif  // TESTS_LOGGENERATOR_HPP
