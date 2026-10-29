// Code from CLP

#ifndef CLP_S_TRACEABLEEXCEPTION_HPP
#define CLP_S_TRACEABLEEXCEPTION_HPP

#include <exception>
#include <string>

#include "ErrorCode.hpp"

// Define a version of __FILE__ that's relative to the source directory
#ifdef SOURCE_PATH_SIZE
    // Temporary until we switch to using C++20's std::source_location
    // NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming)
    #define __FILENAME__ ((__FILE__) + SOURCE_PATH_SIZE)
#else
    // We don't know the source path size, so just default to __FILE__
    // NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming)
    #define __FILENAME__ __FILE__
#endif

namespace clp_s {
class TraceableException : public std::exception {
public:
    // Constructors
    TraceableException(ErrorCode error_code, char const* const filename, int const line_number)
            : m_error_code{error_code},
              m_filename{filename},
              m_line_number{line_number} {
        m_message = std::string(m_filename) + ":" + std::to_string(m_line_number)
                    + "  Error code: " + std::to_string(m_error_code) + "\n";
    }

    // Methods
    [[nodiscard]] auto get_error_code() const -> ErrorCode { return m_error_code; }

    [[nodiscard]] auto get_filename() const -> char const* { return m_filename; }

    [[nodiscard]] auto get_line_number() const -> int { return m_line_number; }

    [[nodiscard]] auto what() const noexcept -> char const* override { return m_message.c_str(); }

private:
    // Variables
    ErrorCode m_error_code;
    char const* m_filename;
    int m_line_number;
    std::string m_message;
};
}  // namespace clp_s
#endif  // CLP_S_TRACEABLEEXCEPTION_HPP
