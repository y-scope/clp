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
#include "BufferedReaderInterface.hpp"
#include "TraceableException.hpp"


class BufferedFileReader : public BufferedReaderInterface {
public:
    static constexpr size_t cDefaultBufferSize = 65536;
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
     * Tries to read a string from the file until it reaches
     * the specified delimiter
     * @param delim The delimiter to stop at
     * @param keep_delimiter Whether to include the delimiter in the
     * output string or not
     * @param append Whether to append to the given string or
     * replace its contents
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

    /**
     * Set the internal buffer
     * @param buffer_size
     * @return ErrorCode_Success on success
     * @return ErrorCode_Failure if the file is not closed
     * @return ErrorCode_BadParam if either:
     *         1. buffer_size is not a integer multiple of 4096
     *         2. buffer_size is not a power of 2
     */
    [[nodiscard]] ErrorCode set_buffer_size(size_t buffer_size);

    /**
     * Peeks the next peek_size bytes of data without advancing the file
     * pos.
     * Note: If further operation such as read or peek is called on the
     * BufferedFileReader after peek_buffered_data, the data_ptr could
     * point to invalid data
     * @param size_to_peek
     * @param data_ptr pointer pointing to peeked data
     * @param peek_size returns number of bytes peeked by reference
     * @return ErrorCode_Success on success
     * @return ErrorCode_errno on error
     * @return ErrorCode_NotInit if the file is not opened
     * @return ErrorCode_EndOfFile if already reaching the eof
     */
    [[nodiscard]] ErrorCode peek_buffered_data(size_t size_to_peek, const char*& data_ptr,
                                               size_t& peek_size);

    /**
     * Tries reading a string view of size = read_size from the ir_buf.
     * @param str_view Returns the string view
     * @param read_size
     * @return true on success, false if the file doesn't contain enough
     * data
     **/
    [[nodiscard]] virtual bool try_read_string_view (MyStringView& str_view,
                                                     size_t read_size) override;

    /**
     * returns a const pointer to the internal buffer
     */
    [[nodiscard]] virtual const char* get_buffer_ptr () override;

    /**
     * Sets a checkpoint pos. the BufferedFileReader guarantees that
     * all data after checkpoint pos will be buffered in the memory and
     * support seek.
     * @return current file pos
     */
    size_t mark_pos();

    /**
     * Disable the checkpoint pos and release buffered data from memory
     */
    void reset_checkpoint ();

private:
    // Methods
    [[nodiscard]] size_t cursor_pos() const { return m_file_pos - m_buffer_begin_pos; }
    [[nodiscard]] char* buffer_head() const { return m_buffer.get() + cursor_pos(); }
    [[nodiscard]] size_t remaining_data_size() const;

    /**
     * Quantize the given size to be the next integer multiple of buffer_size
     * @param size
     * @return quantized size
     */
    [[nodiscard]] size_t quantize_to_buffer_size(size_t size);

    /**
     * Reads next refill_size bytes from file descriptor to the internal buffer
     * and sets the data size of the internal buffer
     * Note: the function returns success even if the number of bytes read from
     * the fd is less than the refill_size
     * @param refill_size
     * @return ErrorCode_Success on success
     * @return ErrorCode_errno on error
     * @return ErrorCode_NotInit if the file is not opened
     * @return ErrorCode_EndOfFile if already reaching the eof
     */
    [[nodiscard]] ErrorCode refill_reader_buffer(size_t refill_size);

    /**
     * Similar to refill_reader_buffer, except that number of bytes refilled
     * is returned by reference
     * @param refill_size
     * @param num_bytes_refilled Returns the number of bytes refilled by
     * reference
     * @return Same as refill_reader_buffer(size_t refill_size)
     */
    [[nodiscard]] ErrorCode refill_reader_buffer(size_t refill_size, size_t& num_bytes_refilled);

    // Types
    size_t m_file_pos;
    int m_fd;
    std::string m_path;

    // Buffer specific data
    std::unique_ptr<char[]> m_buffer;
    size_t m_data_size;
    size_t m_buffer_begin_pos;

    // constant for buffer related calculation
    size_t m_buffer_exp;
    size_t m_buffer_size;
    size_t m_buffer_aligned_mask;

    // Variables for checkpoint support
    bool m_checkpoint_enabled;
    size_t m_checkpoint_pos;
};


#endif // BufferedFileReader
