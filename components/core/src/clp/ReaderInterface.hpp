#ifndef CLP_READERINTERFACE_HPP
#define CLP_READERINTERFACE_HPP

#include <cstddef>
#include <string>

#include "Defs.h"
#include "ErrorCode.hpp"
#include "TraceableException.hpp"

namespace clp {
class ReaderInterface {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override { return "ReaderInterface operation failed"; }
    };

    // Destructor
    virtual ~ReaderInterface() = default;

    // Methods
    virtual ErrorCode try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) = 0;
    virtual ErrorCode try_seek_from_begin(size_t pos) = 0;
    virtual ErrorCode try_get_pos(size_t& pos) = 0;

    /**
     * Tries to read up to the next delimiter and stores it in the given string.
     * NOTE: Implementations should override this if they can achieve better performance.
     * @param delim The delimiter to stop at
     * @param keep_delimiter Whether to include the delimiter in the output string or not
     * @param append Whether to append to the given string or replace its contents
     * @param str The string read
     * @return ErrorCode_Success on success
     * @return Same as ReaderInterface::try_read otherwise
     */
    virtual ErrorCode
    try_read_to_delimiter(char delim, bool keep_delimiter, bool append, std::string& str);

    /**
     * Reads up to a given number of bytes
     * @param buf
     * @param num_bytes_to_read The number of bytes to try and read
     * @param num_bytes_read The actual number of bytes read
     * @return false on EOF
     * @return true otherwise
     */
    bool read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read);

    /**
     * Reads up to the next delimiter and stores it in the given string
     * @param delim The delimiter to stop at
     * @param keep_delimiter Whether to include the delimiter in the output string or not
     * @param append Whether to append to the given string or replace its contents
     * @param str The string read
     * @return false on EOF
     * @return true on success
     */
    bool read_to_delimiter(char delim, bool keep_delimiter, bool append, std::string& str);

    /**
     * Tries to read a number of bytes
     * @param buf
     * @param num_bytes Number of bytes to read
     * @return Same as the underlying medium's try_read method
     * @return ErrorCode_Truncated if 0 < # bytes read < num_bytes
     */
    ErrorCode try_read_exact_length(char* buf, size_t num_bytes);
    /**
     * Reads a number of bytes
     * @param buf
     * @param num_bytes Number of bytes to read
     * @param eof_possible If EOF should be possible (without reading any bytes)
     * @return false if EOF is possible and EOF was hit
     * @return true on success
     */
    bool read_exact_length(char* buf, size_t num_bytes, bool eof_possible);

    /**
     * Tries to read a numeric value from a file
     * @param value The read value
     * @return Same as FileReader::try_read_exact_length's return values
     */
    template <typename ValueType>
    ErrorCode try_read_numeric_value(ValueType& value);
    /**
     * Reads a numeric value
     * @param value The read value
     * @param eof_possible If EOF should be possible (without reading any bytes)
     * @return false if EOF is possible and EOF was hit
     * @return true on success
     */
    template <typename ValueType>
    bool read_numeric_value(ValueType& value, bool eof_possible);

    /**
     * Tries to read a string
     * @param str_length
     * @param str The string read
     * @return Same as ReaderInterface::try_read_exact_length
     */
    ErrorCode try_read_string(size_t str_length, std::string& str);
    /**
     * Reads a string
     * @param str_length
     * @param str The string read
     * @param eof_possible If EOF should be possible (without reading any bytes)
     * @return false if EOF is possible and EOF was hit
     * @return true on success
     */
    bool read_string(size_t str_length, std::string& str, bool eof_possible);

    /**
     * Seeks from the beginning to the given position
     * @param pos
     */
    void seek_from_begin(size_t pos);

    /**
     * Gets the current position of the read head
     * @return Position of the read head
     */
    size_t get_pos();
};

template <typename ValueType>
ErrorCode ReaderInterface::try_read_numeric_value(ValueType& value) {
    ErrorCode error_code = try_read_exact_length(reinterpret_cast<char*>(&value), sizeof(value));
    if (ErrorCode_Success != error_code) {
        return error_code;
    }
    return ErrorCode_Success;
}

template <typename ValueType>
bool ReaderInterface::read_numeric_value(ValueType& value, bool eof_possible) {
    ErrorCode error_code = try_read_numeric_value(value);
    if (ErrorCode_EndOfFile == error_code && eof_possible) {
        return false;
    }
    if (ErrorCode_Success != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
    return true;
}
}  // namespace clp

#endif  // CLP_READERINTERFACE_HPP
