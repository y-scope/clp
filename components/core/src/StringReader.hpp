#ifndef STRINGREADER_HPP
#define STRINGREADER_HPP

// C++ libraries
#include <cstdio>
#include <string>

// Project headers
#include "Defs.h"
#include "ErrorCode.hpp"
#include "ReaderInterface.hpp"
#include "TraceableException.hpp"

class StringReader : public ReaderInterface {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "StringReader operation failed";
        }
    };

    StringReader () : pos(0), m_getdelim_buf_len(0), m_getdelim_buf(nullptr), string_is_set(false) {}
    ~StringReader ();

    // Methods implementing the ReaderInterface
    /**
     * Tries to get the current position of the read head in the file
     * @param pos Position of the read head in the file
     * @return ErrorCode_NotInit if the file is not open
     * @return ErrorCode_errno on error
     * @return ErrorCode_Success on success
     */
    ErrorCode try_get_pos (size_t& pos) override;
    /**
     * Tries to seek from the beginning of the file to the given position
     * @param pos
     * @return ErrorCode_NotInit if the file is not open
     * @return ErrorCode_errno on error
     * @return ErrorCode_Success on success
     */
    ErrorCode try_seek_from_begin (size_t pos) override;

    /**
     * Tries to read up to a given number of bytes from the file
     * @param buf
     * @param num_bytes_to_read The number of bytes to try and read
     * @param num_bytes_read The actual number of bytes read
     * @return ErrorCode_NotInit if the file is not open
     * @return ErrorCode_BadParam if buf is invalid
     * @return ErrorCode_errno on error
     * @return ErrorCode_EndOfFile on EOF
     * @return ErrorCode_Success on success
     */
    ErrorCode try_read (char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) override;

    // Methods
    bool is_open () const { return string_is_set; }
    /**
     * Tries to open a file
     * @param path
     * @return ErrorCode_Success on success
     * @return ErrorCode_FileNotFound if the file was not found
     * @return ErrorCode_errno otherwise
     */
    ErrorCode try_open (const std::string& input_string);
    /**
     * Opens a file
     * @param path
     * @throw StringReader::OperationFailed on failure
     */
    void open (const std::string& input_string);
    /**
     * Closes the file if it's open
     */
    void close ();
    /**
     * Tries to stat the current file
     * @param stat_buffer
     * @return ErrorCode_errno on error
     * @return ErrorCode_Success on success
     */
private:
    size_t m_getdelim_buf_len;
    char* m_getdelim_buf;
    std::string input_string;
    uint32_t pos;
    bool string_is_set;
};

#endif // STRINGREADER_HPP
