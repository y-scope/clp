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

    // User <int> roles: <roles>.
    // Transaction <int> amount <float> processed.
    // ALERT! <enum(CRITICAL, WARNING, INFO)> severity <enum(HIGH, MEDIUM, LOW)> in system!
    // User <int> attempted to login <int> times.
    // Service <string> responded in <int> ms.
    // Order <int> is <enum(PENDING,SHIPPED,CANCELLED)>; items <Items>.
    // Module <string> generated event <enum(START,STOP,ERROR,INFO)>.
    // Payment by <int> via <enum(CASH,CARD,PAYPAL)> success=<bool>.
    // Backup <int> took <int> ms; size: <float> GB.
    // user_id=<int> action=<enum(LOGIN, LOGOUT, UPLOAD, DOWNLOAD, DELETE, VIEW)>.
    // error_code=<int> The user disconnected.
    // order_id=<int> items=<Items>.
    // The user_id=<int> has optional_field=<Fields>.
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
    if (VarType::Int == type) return std::to_string(1);
    if (VarType::Float == type) return std::to_string(0.1 * 1);
    if (VarType::UserAction == type) return "LOGIN"; // LOGIN, LOGOUT, UPLOAD, DOWNLOAD, DELETE, VIEW
    if (VarType::Fields == type) return "{abc, def, ghi}";
    if (VarType::Items == type) return "{abc, def, ghi}";
    if (VarType::Path == type) return "abc/def/ghi";
    if (VarType::Ip == type) return "111.111.1.1";
    if (VarType::Connection == type) return "GET"; // GET,POST,PUT,DELETE
    // if ("string" == type) return "abc";
    // if ("bool" == type) return "true";
    return "unknown";
}
