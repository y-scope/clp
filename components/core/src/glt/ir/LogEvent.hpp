#ifndef GLT_IR_LOGEVENT_HPP
#define GLT_IR_LOGEVENT_HPP

#include <string>
#include <vector>

#include "../Defs.h"
#include "types.hpp"

namespace glt::ir {
/**
 * A class representing a log event encoded using CLP's IR
 * @tparam encoded_variable_t The type of encoded variables in the event
 */
template <typename encoded_variable_t>
class LogEvent {
public:
    // Constructors
    LogEvent(
            epoch_time_ms_t timestamp,
            std::string logtype,
            std::vector<std::string> dict_vars,
            std::vector<encoded_variable_t> encoded_vars
    )
            : m_timestamp{timestamp},
              m_logtype{std::move(logtype)},
              m_dict_vars{std::move(dict_vars)},
              m_encoded_vars{std::move(encoded_vars)} {}

    // Methods
    [[nodiscard]] auto get_timestamp() const -> epoch_time_ms_t { return m_timestamp; }

    [[nodiscard]] auto get_logtype() const -> std::string const& { return m_logtype; }

    [[nodiscard]] auto get_dict_vars() const -> std::vector<std::string> const& {
        return m_dict_vars;
    }

    [[nodiscard]] auto get_encoded_vars() const -> std::vector<encoded_variable_t> const& {
        return m_encoded_vars;
    }

private:
    // Variables
    epoch_time_ms_t m_timestamp;
    std::string m_logtype;
    std::vector<std::string> m_dict_vars;
    std::vector<encoded_variable_t> m_encoded_vars;
};
}  // namespace glt::ir

#endif  // GLT_IR_LOGEVENT_HPP
