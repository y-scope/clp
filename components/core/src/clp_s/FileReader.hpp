// Code from CLP

#ifndef CLP_S_FILEREADER_HPP
#define CLP_S_FILEREADER_HPP

#include <cstdio>
#include <string>

#include "ErrorCode.hpp"
#include "TraceableException.hpp"

namespace clp_s {
class FileReader {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructor
    FileReader() : m_file(nullptr), m_getdelim_buf_len(0), m_getdelim_buf(nullptr) {}

    // Destructor
    ~FileReader();

    // Methods implementing the ReaderInterface
    /**
     * Tries to get the current position of the read head in the file
     * @param pos Position of the read head in the file
     * @return ErrorCodeNotInit if the file is not open
     * @return ErrorCodeErrno on error
     * @return ErrorCodeSuccess on success
     */
    ErrorCode try_get_pos(size_t& pos);

    /**
     * Tries to seek from the beginning of the file to the given position
     * @param pos The position to seek to
     * @return ErrorCodeNotInit if the file is not open
     * @return ErrorCodeErrno on error
     * @return ErrorCodeSuccess on success
     */
    ErrorCode try_seek_from_begin(size_t pos);

    /**
     * Tries to read up to a given number of bytes from the file
     * @param buf The buffer to read into
     * @param num_bytes_to_read The number of bytes to try and read
     * @param num_bytes_read The actual number of bytes read
     * @return ErrorCodeNotInit if the file is not open
     * @return ErrorCodeBadParam if buf is invalid
     * @return ErrorCodeErrno on error
     * @return ErrorCodeEndOfFile on EOF
     * @return ErrorCodeSuccess on success
     */
    ErrorCode try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read);

    /**
     * Tries to read a string from the file until it reaches the specified delimiter
     * @param delim The delimiter to stop at
     * @param keep_delimiter Whether to include the delimiter in the output string or not
     * @param append Whether to append to the given string or replace its contents
     * @param str The string read
     * @return ErrorCodeSuccess on success
     * @return ErrorCodeEndOfFile on EOF
     * @return ErrorCodeErrno otherwise
     */
    ErrorCode try_read_to_delimiter(char delim, bool keep_delimiter, bool append, std::string& str);

    /**
     * Tries to read a number of bytes
     * @param buf The buffer to read into
     * @param num_bytes Number of bytes to read
     * @return Same as the underlying medium's try_read method
     * @return ErrorCodeTruncated if 0 < # bytes read < num_bytes
     */
    ErrorCode try_read_exact_length(char* buf, size_t num_bytes);

    /**
     * Tries to read a numeric value
     * @tparam ValueType The type of the value to read
     * @param value The value read
     * @return Same as the underlying medium's try_read_exact_length method
     */
    template <typename ValueType>
    ErrorCode try_read_numeric_value(ValueType& value) {
        ErrorCode error_code
                = try_read_exact_length(reinterpret_cast<char*>(&value), sizeof(value));
        if (ErrorCodeSuccess != error_code) {
            return error_code;
        }
        return ErrorCodeSuccess;
    }

    /**
     * Reads a numeric value
     * @tparam ValueType The type of the value to read
     * @param value The value read
     * @param eof_possible Whether EOF is possible or not
     * @return true on success
     * @return false on EOF if eof_possible is true
     * @throw FileReader::OperationFailed on failure
     */
    template <typename ValueType>
    bool read_numeric_value(ValueType& value, bool eof_possible) {
        ErrorCode error_code = try_read_numeric_value(value);
        if (ErrorCodeEndOfFile == error_code && eof_possible) {
            return false;
        }
        if (ErrorCodeSuccess != error_code) {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }
        return true;
    }

    // Methods
    /**
     * Checks if the file is open
     * @return true if the file is open, false otherwise
     */
    bool is_open() const { return m_file != nullptr; }

    /**
     * Tries to open a file
     * @param path
     * @return ErrorCodeSuccess on success
     * @return ErrorCodeFileNotFound if the file was not found
     * @return ErrorCodeErrno otherwise
     */
    ErrorCode try_open(std::string const& path);

    /**
     * Opens a file
     * @param path
     * @throw FileReader::OperationFailed on failure
     */
    void open(std::string const& path);

    /**
     * Closes the file if it's open
     */
    void close();

    /**
     * Gets the current position of the read head
     * @return Position of the read head
     */
    size_t get_pos();

    /**
     * Seeks from the beginning to the given position
     * @param pos
     */
    void seek_from_begin(size_t pos);

private:
    FILE* m_file;
    size_t m_getdelim_buf_len;
    char* m_getdelim_buf;
};
}  // namespace clp_s

#endif  // CLP_S_FILEREADER_HPP
