#ifndef TESTS_LOGTYPE_HPP
#define TESTS_LOGTYPE_HPP

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

enum class VarType : uint8_t {
    Int = 0,
    Float,
    UserAction, // LOGIN, LOGOUT, UPLOAD, DOWNLOAD, DELETE, VIEW
    Fields,
    Items,
    Path,
    Ip,
    Connection // GET,POST,PUT,DELETE
};

using LogType = std::vector<std::variant<std::string, VarType>>;

#endif  // TESTS_LOGTYPE_HPP
