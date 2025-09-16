#ifndef CLP_IR_LOGEVENT_HPP
#define CLP_IR_LOGEVENT_HPP

#include <string>
#include <utility>
#include <vector>

#include "../time_types.hpp"
#include "EncodedTextAst.hpp"
#include "types.hpp"

namespace clp::ir {
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
            UtcOffset utc_offset,
            EncodedTextAst<encoded_variable_t> message
    )
            : m_timestamp{timestamp},
              m_utc_offset{utc_offset},
              m_message{std::move(message)} {}

    // Methods
    [[nodiscard]] auto get_timestamp() const -> epoch_time_ms_t { return m_timestamp; }

    [[nodiscard]] auto get_utc_offset() const -> UtcOffset { return m_utc_offset; }

    [[nodiscard]] auto get_message() const -> EncodedTextAst<encoded_variable_t> const& {
        return m_message;
    }

private:
    // Variables
    epoch_time_ms_t m_timestamp{0};
    UtcOffset m_utc_offset{0};
    EncodedTextAst<encoded_variable_t> m_message;
};
}  // namespace clp::ir

#endif  // CLP_IR_LOGEVENT_HPP
