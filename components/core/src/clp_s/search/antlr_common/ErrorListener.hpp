#ifndef CLP_S_SEARCH_ANTLRCOMMON_ERRORLISTENER_HPP
#define CLP_S_SEARCH_ANTLRCOMMON_ERRORLISTENER_HPP

#include <cstddef>
#include <exception>
#include <string>
#include <string_view>

#include <antlr4-runtime.h>

namespace clp_s::search::antlr_common {
class ErrorListener : public antlr4::BaseErrorListener {
public:
    auto syntaxError(
            [[maybe_unused]] antlr4::Recognizer* recognizer,
            [[maybe_unused]] antlr4::Token* offending_symbol,
            [[maybe_unused]] size_t line,
            [[maybe_unused]] size_t char_position_in_line,
            std::string const& msg,
            [[maybe_unused]] std::exception_ptr e
    ) -> void override {
        m_error = true;
        m_error_message = msg;
    }

    [[nodiscard]] auto error() const -> bool { return m_error; }

    [[nodiscard]] auto message() const -> std::string_view { return m_error_message; }

private:
    bool m_error{false};
    std::string m_error_message;
};
}  // namespace clp_s::search::antlr_common

#endif  // CLP_S_SEARCH_ANTLRCOMMON_ERRORLISTENER_HPP
