#ifndef TRACEABLEEXCEPTION
#define TRACEABLEEXCEPTION

// C++ libraries
#include <exception>

// Project headers
#include "ErrorCode.hpp"

class TraceableException : public std::exception {
public:
    // Constructors
    TraceableException (ErrorCode error_code, const char* const filename, const int line_number) : m_error_code(error_code), m_filename(filename),
            m_line_number(line_number) {}

    // Copy constructor / assignment operators
    TraceableException (const TraceableException&) = default;
    TraceableException& operator= (const TraceableException&) = default;

    // Methods
    ErrorCode get_error_code () const { return m_error_code; }
    const char* get_filename () const { return m_filename; }
    int get_line_number () const { return m_line_number; }

    // NOTE: We make what() abstract to make the entire class abstract
    virtual const char* what () const noexcept = 0;

private:
    // Variables
    ErrorCode m_error_code;
    const char* m_filename;
    int m_line_number;
};

// Macros
// Relative version of __FILE__
#define __FILENAME__ ((__FILE__) + SOURCE_PATH_SIZE)

#endif // TRACEABLEEXCEPTION
