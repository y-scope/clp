#include "LogGenerator.hpp"

#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include "LogType.hpp"

using std::array;
using std::string;
using std::uniform_int_distribution;
using std::vector;

LogGenerator::LogGenerator(uint32_t const seed) : m_rng(seed) {
    m_log_types.emplace_back(LogType{"The process took ", VarType::Int, " ms to complete."});
    m_log_types.emplace_back(
            LogType{"User ", VarType::Int, " did ", VarType::UserAction, " unexpectedly."}
    );
    m_log_types.emplace_back(LogType{"Oops! error ", VarType::Int, " occured for ", VarType::Int});
    m_log_types.emplace_back(LogType{"Order ", VarType::Int, " contains items: ", VarType::List});
    m_log_types.emplace_back(LogType{"User ", VarType::Int, " has extra fields: ", VarType::List});
    m_log_types.emplace_back(LogType{"Session ", VarType::Int, " lasted ", VarType::Int, " mins."});
    m_log_types.emplace_back(
            LogType{"User ", VarType::Int, " clicked ", VarType::Path, VarType::Int, " times."}
    );
    m_log_types.emplace_back(
            LogType{"File ", VarType::Path, " failed, size=", VarType::Int, " bytes."}
    );
    m_log_types.emplace_back(
            LogType{VarType::Ip, " connected using ", VarType::Connection, " to ", VarType::Path}
    );
    m_log_types.emplace_back(
            LogType{"CPU at ", VarType::Float, "% memory at ", VarType::Float, "%!"}
    );
    m_log_types.emplace_back(LogType{"User ", VarType::Int, " roles: ", VarType::Roles});
    m_log_types.emplace_back(
            LogType{"Transaction ", VarType::Int, " amount ", VarType::Float, " processed."}
    );
    m_log_types.emplace_back(
            LogType{"Alert ", VarType::Severity, " severity ", VarType::Level, " in system!"}
    );
    m_log_types.emplace_back(LogType{"Alert ", VarType::Severity, " severity ", VarType::Roles});
    m_log_types.emplace_back(
            LogType{"User ", VarType::Int, " attempted to login ", VarType::Int, " times."}
    );
    m_log_types.emplace_back(
            LogType{"Service ", VarType::Name, " responded in ", VarType::Int, "ms."}
    );
    m_log_types.emplace_back(
            LogType{"Order ", VarType::Int, " is ", VarType::Status, "; ", VarType::List, "."}
    );
    m_log_types.emplace_back(
            LogType{"Module ", VarType::Name, " generated event ", VarType::Event, "."}
    );
    m_log_types.emplace_back(
            LogType{"Payment by ", VarType::Int, " via ", VarType::Pay, " success=", VarType::Bool}
    );
    m_log_types.emplace_back(
            LogType{"Backup ", VarType::Int, " took ", VarType::Int, " ms; ", VarType::Float, "GB."}
    );
    m_log_types.emplace_back(LogType{"user_id=", VarType::Int, ", action=", VarType::UserAction});
    m_log_types.emplace_back(LogType{"error_code=", VarType::Int, " The user disconnected."});
    m_log_types.emplace_back(LogType{"orer_id=", VarType::Int, ", items= ", VarType::List});
    m_log_types.emplace_back(
            LogType{"The user_id=", VarType::Int, " has optional_fields=", VarType::List}
    );
}

auto LogGenerator::generate_logs(size_t const count) -> vector<string> {
    vector<string> logs;
    for (uint32_t i{0}; i < count; i++) {
        uniform_int_distribution<size_t> dist(0, m_log_types.size() - 1);
        auto const log_type_id{dist(m_rng)};
        logs.emplace_back();
        auto& log{logs.back()};
        for (auto const& token : m_log_types[log_type_id]) {
            if (std::holds_alternative<std::string>(token)) {
                log += std::get<string>(token);
            } else {
                log += generate_value(std::get<VarType>(token));
            }
        }
    }
    return logs;
}

auto LogGenerator::generate_value(VarType const type) -> std::string {
    if (VarType::Int == type) {
        uniform_int_distribution<size_t> encodable_dist(0, 1);
        if (1 == encodable_dist(m_rng)) {
            uniform_int_distribution<uint64_t> dist(0, std::numeric_limits<uint64_t>::max());
            return std::to_string(dist(m_rng));
        }
        uniform_int_distribution<size_t> len_dist(21, 40);
        uniform_int_distribution<int> digit_dist(0, 9);
        string result;
        auto length{len_dist(m_rng)};
        for (size_t i{0}; i < length; ++i) {
            result += static_cast<char>('0' + digit_dist(m_rng));
        }
        return result;
    }
    if (VarType::Float == type) {
        uniform_int_distribution<size_t> encodable_dist(0, 1);
        uniform_int_distribution<int> digit_dist(0, 9);
        size_t length;
        if (1 == encodable_dist(m_rng)) {
            uniform_int_distribution<size_t> len_dist(2, 17);
            length = len_dist(m_rng);
        } else {
            uniform_int_distribution<size_t> len_dist(18, 40);
            length = len_dist(m_rng);
        }
        uniform_int_distribution<size_t> decimal_dist(1, length - 1);
        auto decimal_place{decimal_dist(m_rng)};
        string result;
        for (size_t i{0}; i < length + 1; ++i) {
            if (decimal_place == i) {
                result += '.';
                continue;
            }
            result += static_cast<char>('0' + digit_dist(m_rng));
        }
        return result;
    }
    if (VarType::UserAction == type) {
        static const array<string, 6> cValues{
                "LOGIN", "LOGOUT", "UPLOAD", "DOWNLOAD", "DELETE", "VIEW"
        };
        uniform_int_distribution<size_t> dist(0, cValues.size() - 1);
        return cValues[dist(m_rng)];
    }
    if (VarType::List == type) {
        uniform_int_distribution<size_t> num_fields_dist(1, 10);
        auto num_fields{num_fields_dist(m_rng)};
        string result{"{"};
        for (size_t i{0}; i < num_fields; ++i) {
            result += generate_string();
            if (i < num_fields - 1) {
                result += ", ";
            }
        }
        result += "}";
        return result;
    }
    if (VarType::Path == type) {
        uniform_int_distribution<size_t> num_paths_dist(1, 5);
        auto num_paths{num_paths_dist(m_rng)};
        string result;
        for (size_t i{0}; i < num_paths; ++i) {
            result += generate_string();
            if (i < num_paths - 1) {
                result += "/";
            }
        }
        return result;
    }
    if (VarType::Ip == type) {
        uniform_int_distribution<size_t> octet_dist(1, 254);
        auto result{std::to_string(octet_dist(m_rng))};
        result += "." + std::to_string(octet_dist(m_rng));
        result += "." + std::to_string(octet_dist(m_rng));
        result += "." + std::to_string(octet_dist(m_rng));
        return result;
    }
    if (VarType::Connection == type) {
        static const array<string, 4> cValues{"GET", "POST", "PUT", "DELETE"};
        uniform_int_distribution<size_t> dist(0, cValues.size() - 1);
        return cValues[dist(m_rng)];
    }
    if (VarType::Roles == type) {
        static const array<string, 6> cValues{
                "User", "Admin", "Moderator", "Guest", "SuperUser", "Service"
        };
        uniform_int_distribution<size_t> dist(0, cValues.size() - 1);
        return cValues[dist(m_rng)];
    }
    if (VarType::Severity == type) {
        static const array<string, 3> cValues{"CRITICAL", "WARNING", "INFO"};
        uniform_int_distribution<size_t> dist(0, cValues.size() - 1);
        return cValues[dist(m_rng)];
    }
    if (VarType::Level == type) {
        static const array<string, 3> cValues{"HIGH", "MEDIUM", "LOW"};
        uniform_int_distribution<size_t> dist(0, cValues.size() - 1);
        return cValues[dist(m_rng)];
    }
    if (VarType::Name == type) {
        return generate_string();
    }
    if (VarType::Status == type) {
        static const array<string, 3> cValues{"PENDING", "SHIPPED", "CANCELLED"};
        uniform_int_distribution<size_t> dist(0, cValues.size() - 1);
        return cValues[dist(m_rng)];
    }
    if (VarType::Event == type) {
        static const array<string, 4> cValues{"START", "STOP", "ERROR", "INFO"};
        uniform_int_distribution<size_t> dist(0, cValues.size() - 1);
        return cValues[dist(m_rng)];
    }
    if (VarType::Pay == type) {
        static const array<string, 3> cValues{"CASH", "CARD", "PAYPAL"};
        uniform_int_distribution<size_t> dist(0, cValues.size() - 1);
        return cValues[dist(m_rng)];
    }
    if (VarType::Bool == type) {
        static const array<string, 2> cValues{"TRUE", "FALSE"};
        uniform_int_distribution<size_t> dist(0, cValues.size() - 1);
        return cValues[dist(m_rng)];
    }
    return "unknown";
}

auto LogGenerator::generate_string() -> std::string {
    static const string cChars{"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"};
    uniform_int_distribution<size_t> len_dist(1, 20);
    auto length{len_dist(m_rng)};

    uniform_int_distribution<size_t> value_dist(0, cChars.size() - 1);

    string result;
    result.reserve(length);
    for (size_t i{0}; i < length; ++i) {
        result += cChars[value_dist(m_rng)];
    }
    return result;
}
