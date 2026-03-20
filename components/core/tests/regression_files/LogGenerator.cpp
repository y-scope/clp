#include "LogGenerator.hpp"

#include <cstdint>
#include <string>
#include <vector>

#include "LogType.hpp"

using std::string;
using std::vector;

LogGenerator::LogGenerator(uint32_t const seed) : m_seed(seed) {
    m_log_types.emplace_back(LogType{"The process took ", VarType::Int, " ms to complete."});
    m_log_types.emplace_back(
            LogType{"User ", VarType::Int, " did ", VarType::UserAction, " unexpectedly."}
    );
    m_log_types.emplace_back(LogType{"Oops! error ", VarType::Int, " occured for ", VarType::Int});
    m_log_types.emplace_back(LogType{"Order ", VarType::Int, " contains items: ", VarType::Items});
    m_log_types.emplace_back(
            LogType{"User ", VarType::Int, " has extra fields: ", VarType::Fields}
    );
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
            LogType{"Order ", VarType::Int, " is ", VarType::Status, "; ", VarType::Items, "."}
    );
    m_log_types.emplace_back(
            LogType{"Module ", VarType::Name, " generated event ", VarType::Event, "."}
    );
    m_log_types.emplace_back(
            LogType{"Payment by ", VarType::Int, " via ", VarType::Pay, " success=.", VarType::Bool}
    );
    m_log_types.emplace_back(
            LogType{"Backup ", VarType::Int, " took ", VarType::Int, " ms; ", VarType::Float, "GB."}
    );
    m_log_types.emplace_back(LogType{"user_id=", VarType::Int, ", action=", VarType::UserAction});
    m_log_types.emplace_back(LogType{"error_code=", VarType::Int, " The user disconnected."});
    m_log_types.emplace_back(LogType{"orer_id=", VarType::Int, ", items= ", VarType::Items});
    m_log_types.emplace_back(
            LogType{"The user_id=", VarType::Int, " has optional_fields=", VarType::Fields}
    );
}

auto LogGenerator::generate_logs(size_t const count) -> vector<string> {
    vector<string> logs;
    for (uint32_t i{0}; i < count; i++) {
        auto log_type_id{i % m_log_types.size()};
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
        return std::to_string(1);
    }
    if (VarType::Float == type) {
        return std::to_string(0.1 * 1);
    }
    if (VarType::UserAction == type) {
        return "LOGIN"; // LOGIN, LOGOUT, UPLOAD, DOWNLOAD, DELETE, VIEW
    }
    if (VarType::Fields == type) {
        return "{abc, def, ghi}";
    }
    if (VarType::Items == type) {
        return "{abc, def, ghi}";
    }
    if (VarType::Path == type) {
        return "abc/def/ghi";
    }
    if (VarType::Ip == type) {
        return "111.111.1.1";
    }
    if (VarType::Connection == type) {
        return "GET"; // GET,POST,PUT,DELETE
    }
    if (VarType::Roles == type) {
        return "User"; //
    }
    if (VarType::Severity == type) {
        return "CRITICAL"; // CRITICAL, WARNING, INFO
    }
    if (VarType::Level == type) {
        return "HIGH"; // HIGH, MEDIUM, LOW
    }
    if (VarType::Name == type) {
        return "Name"; // any string
    }
    if (VarType::Status == type) {
        return "PENDING"; // PENDING, SHIPPED, CANCELLED
    }
    if (VarType::Event == type) {
        return "START"; // START, STOP, ERROR, INFO
    }
    if (VarType::Pay == type) {
        return "CASH"; // CASH, CARD, PAYPAL
    }
    if (VarType::Bool == type) {
        return "TRUE"; // TRUE, FALSE
    }
    return "unknown";
}
