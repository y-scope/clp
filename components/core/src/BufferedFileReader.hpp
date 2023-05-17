#ifndef BufferedFileReader_HPP
#define BufferedFileReader_HPP

// C standard libraries

// C++ libraries
#include <cstdio>
#include <memory>
#include <string>

// Project headers
#include "Defs.h"
#include "ErrorCode.hpp"
#include "ReaderInterface.hpp"
#include "TraceableException.hpp"


class BufferedFileReader : public ReaderInterface {
public:
    static constexpr size_t DefaultBufferSize = 65536;
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) :
            TraceableException (error_code, filename, line_number) {}

        // Methods
        [[nodiscard]] const char* what () const noexcept override {
            return "BufferedFileReader operation failed";
        }
    };

    // Constructors
    BufferedFileReader();
    ~BufferedFileReader();
    // Methods implementing the ReaderInterface
    /**
     * Tries to get the current position of the read head in the file
     * @param pos Position of the read head in the file
     * @return ErrorCode_NotInit if the file is not open
     * @return ErrorCode_errno on error
     * @return ErrorCode_Success on success
     */
    [[nodiscard]] ErrorCode try_get_pos (size_t& pos) override;
    /**
     * Tries to seek from the beginning of the file to the given position
     * @param pos
     * @return ErrorCode_NotInit if the file is not open
     * @return ErrorCode_errno on error
     * @return ErrorCode_Success on success
     */
    [[nodiscard]] ErrorCode try_seek_from_begin (size_t pos) override;

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
    [[nodiscard]] ErrorCode try_read (char* buf, size_t num_bytes_to_read,
                                      size_t& num_bytes_read) override;

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
    [[nodiscard]] ErrorCode try_read_to_delimiter (char delim, bool keep_delimiter,
                                                   bool append, std::string& str) override;

    // Methods
    [[nodiscard]] bool is_open () const { return -1 != m_fd; }

    /**
     * Tries to open a file
     * @param path
     * @return ErrorCode_Success on success
     * @return ErrorCode_FileNotFound if the file was not found
     * @return ErrorCode_errno otherwise
     */
    [[nodiscard]] ErrorCode try_open (const std::string& path);
    /**
     * Opens a file
     * @param path
     * @throw BufferedFileReader::OperationFailed on failure
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
    [[nodiscard]] ErrorCode try_fstat (struct stat& stat_buffer) const;

    size_t mark_pos();
    void revert_pos();
    void reset_checkpoint ();
    [[nodiscard]] ErrorCode set_buffer_size(size_t buffer_size);

private:
    [[nodiscard]] size_t cursor_pos() { return m_file_pos - m_buffer_begin_pos; }
    [[nodiscard]] size_t remaining_data_size();
    [[nodiscard]] int8_t* buffer_head() {
        return m_buffer.get() + cursor_pos();
    }
    [[nodiscard]] ErrorCode refill_reader_buffer(size_t refill_size);
    [[nodiscard]] ErrorCode refill_reader_buffer(size_t refill_size, size_t& num_bytes_refilled);

    // Types
    size_t m_file_pos;
    int m_fd;
    std::string m_path;

    // Buffer specific data
    std::unique_ptr<int8_t[]> m_buffer;
    size_t m_size;
    size_t m_buffer_begin_pos;

    // constant flag
    size_t m_reader_buffer_exp;
    size_t m_reader_buffer_size;
    size_t m_reader_buffer_aligned_mask;
    size_t m_reader_buffer_cursor_mask;
    // checkpoint specific data
    bool m_checkpoint_enabled;
    size_t m_checkpoint_pos;
    size_t m_checkpointed_buffer_pos;
};


#endif // BufferedFileReader
