#ifndef CLP_TRACEABLEEXCEPTION_HPP
#define CLP_TRACEABLEEXCEPTION_HPP

#include <exception>

#include "ErrorCode.hpp"

namespace clp {
class TraceableException : public std::exception {
public:
    // Constructors
    TraceableException(ErrorCode error_code, char const* const filename, int const line_number)
            : m_error_code(error_code),
              m_filename(filename),
              m_line_number(line_number) {}

    // Copy constructor / assignment operators
    TraceableException(TraceableException const&) = default;
    TraceableException& operator=(TraceableException const&) = default;

    // Methods
    ErrorCode get_error_code() const { return m_error_code; }

    char const* get_filename() const { return m_filename; }

    int get_line_number() const { return m_line_number; }

    // NOTE: We make what() abstract to make the entire class abstract
    virtual char const* what() const noexcept = 0;

private:
    // Variables
    ErrorCode m_error_code;
    char const* m_filename;
    int m_line_number;
};
}  // namespace clp

// Macros
// Define a version of __FILE__ that's relative to the source directory
#ifdef SOURCE_PATH_SIZE
   // NOLINTNEXTLINE
    #define __FILENAME__ ((__FILE__) + SOURCE_PATH_SIZE)
#else
   // We don't know the source path size, so just default to __FILE__
    #define __FILENAME__ __FILE__
#endif

#endif  // CLP_TRACEABLEEXCEPTION_HPP
