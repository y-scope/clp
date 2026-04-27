#ifndef CLP_S_FFI_SFA_LOGEVENT_HPP
#define CLP_S_FFI_SFA_LOGEVENT_HPP

#include <cstdint>
#include <string>
#include <utility>

namespace clp_s::ffi::sfa {
class LogEvent {
public:
    LogEvent(int64_t log_event_idx, int64_t timestamp, std::string message)
            : m_log_event_idx{log_event_idx},
              m_timestamp{timestamp},
              m_message{std::move(message)} {}

    [[nodiscard]] auto get_log_event_idx() const -> int64_t { return m_log_event_idx; }

    [[nodiscard]] auto get_timestamp() const -> int64_t { return m_timestamp; }

    [[nodiscard]] auto get_message() const -> std::string const& { return m_message; }

private:
    int64_t m_log_event_idx{0};
    int64_t m_timestamp{0};
    std::string m_message;
};
}  // namespace clp_s::ffi::sfa

#endif  // CLP_S_FFI_SFA_LOGEVENT_HPP
