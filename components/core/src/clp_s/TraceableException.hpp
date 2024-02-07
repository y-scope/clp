// Code from CLP

#ifndef CLP_S_TRACEABLEEXCEPTION_HPP
#define CLP_S_TRACEABLEEXCEPTION_HPP

#include <exception>
#include <string>

#include "ErrorCode.hpp"

#define __FILENAME__ ((__FILE__) + SOURCE_PATH_SIZE)

namespace clp_s {
class TraceableException : public std::exception {
public:
    // Constructors
    TraceableException(ErrorCode error_code, char const* const filename, int const line_number)
            : m_error_code(error_code),
              m_filename(filename),
              m_line_number(line_number) {
        m_message += std::string(m_filename) + ":" + std::to_string(m_line_number)
                     + "  Error code: " + std::to_string(m_error_code) + "\n";
    }

    // Copy constructor / assignment operators
    TraceableException(TraceableException const&) = default;
    TraceableException& operator=(TraceableException const&) = default;

    // Methods
    ErrorCode get_error_code() const { return m_error_code; }

    char const* get_filename() const { return m_filename; }

    int get_line_number() const { return m_line_number; }

    char const* what() const noexcept override { return m_message.c_str(); }

protected:
    std::string m_message;

private:
    // Variables
    ErrorCode m_error_code;
    char const* m_filename;
    int m_line_number;
};
}  // namespace clp_s

#endif  // CLP_S_TRACEABLEEXCEPTION_HPP
