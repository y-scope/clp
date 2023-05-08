#ifndef FileReaderSys_HPP
#define FileReaderSys_HPP

// C standard libraries

// C++ libraries
#include <cstdio>
#include <memory>
#include <string>

// Project headers
#include "Defs.h"
#include "ErrorCode.hpp"
#include "BufferReader.hpp"
#include "TraceableException.hpp"


class FileReader : public BufferReader {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "FileReader operation failed";
        }
    };

    // Constructors
    FileReader() : m_file_pos(0), m_buffer_pos(0), m_fd(-1) {
        m_read_buffer = reinterpret_cast<int8_t*>(malloc(sizeof(int8_t) * cReaderBufferSize));
    }
    ~FileReader();
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

    /**
     * Tries to read a string from the file until it reaches the specified delimiter
     * @param delim The delimiter to stop at
     * @param keep_delimiter Whether to include the delimiter in the output string or not
     * @param append Whether to append to the given string or replace its contents
     * @param str The string read
     * @return ErrorCode_Success on success
     * @return ErrorCode_EndOfFile on EOF
     * @return ErrorCode_errno otherwise
     */
    ErrorCode try_read_to_delimiter (char delim, bool keep_delimiter, bool append, std::string& str) override;

    // Methods
    bool is_open () const { return -1 != m_fd; }

    /**
     * Tries to open a file
     * @param path
     * @return ErrorCode_Success on success
     * @return ErrorCode_FileNotFound if the file was not found
     * @return ErrorCode_errno otherwise
     */
    ErrorCode try_open (const std::string& path);
    /**
     * Opens a file
     * @param path
     * @throw FileReader::OperationFailed on failure
     */
    void open (const std::string& path);
    /**
     * Closes the file if it's open
     */
    void close ();

    [[nodiscard]] const std::string& get_path () const { return m_path; }

    /**
     * Tries to stat the current file
     * @param stat_buffer
     * @return ErrorCode_errno on error
     * @return ErrorCode_Success on success
     */
    ErrorCode try_fstat (struct stat& stat_buffer);


private:

    ErrorCode refill_reader_buffer(size_t num_bytes_to_read);

    // Types
    size_t m_file_pos;
    int m_fd;
    std::string m_path;
    bool reached_eof;
    bool started_reading;

    // Buffer specific data
    ssize_t m_buffer_length;
    size_t m_buffer_pos;
    int8_t* m_read_buffer;
    static constexpr size_t cReaderBufferSize = 1024;
};


#endif // FileReaderSys_HPP
