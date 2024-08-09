#ifndef CLP_S_SEARCH_ANTLRCOMMON_ERRORLISTENER_HPP
#define CLP_S_SEARCH_ANTLRCOMMON_ERRORLISTENER_HPP
#include <antlr4-runtime.h>

namespace clp_s::search::antlr_common {
class ErrorListener : public antlr4::BaseErrorListener {
public:
    void syntaxError(
            antlr4::Recognizer* recognizer,
            antlr4::Token* offending_symbol,
            size_t line,
            size_t char_position_in_line,
            std::string const& msg,
            std::exception_ptr e
    ) override {
        m_error = true;
        m_error_message = msg;
    }

    bool error() const { return m_error; }

    std::string const& message() const { return m_error_message; }

private:
    bool m_error{false};
    std::string m_error_message;
};
}  // namespace clp_s::search::antlr_common

#endif  // CLP_S_SEARCH_ANTLRCOMMON_ERRORLISTENER_HPP
