#ifndef TESTS_LOGTYPE_HPP
#define TESTS_LOGTYPE_HPP

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

enum class VarType : uint8_t {
    Int = 0,
    Float,
    UserAction,
    Fields,
    Items,
    Path,
    Ip,
    Connection,
    Roles,
    Severity,
    Level,
    Name,
    Status,
    Event,
    Pay,
    Bool
};

using LogType = std::vector<std::variant<std::string, VarType>>;

#endif  // TESTS_LOGTYPE_HPP
