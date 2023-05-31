#ifndef BUFFERREADER_HPP
#define BUFFERREADER_HPP

// Project headers
#include "ReaderInterface.hpp"

/**
 * Class for reading from a fixed size in memory buffer
 */
class BufferReader : public ReaderInterface {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) :
            TraceableException (error_code, filename, line_number) {}

        // Methods
        [[nodiscard]] const char* what () const noexcept override {
            return "BufferReader operation failed";
        }
    };

    // Constructors
    BufferReader () : m_data(nullptr), m_data_size(0), m_cursor_pos(0) {}
    BufferReader (const char* data, size_t size) : m_data(data), m_data_size(size), m_cursor_pos(0) {}

    // Methods implementing the ReaderInterface
    /**
     * Tries to read up to a given number of bytes from the buffer
     * @param buf
     * @param num_bytes_to_read The number of bytes to try and read
     * @param num_bytes_read The actual number of bytes read
     * @return ErrorCode_NotInit if the buffer is not initialized
     * @return ErrorCode_BadParam if buf is invalid
     * @return ErrorCode_EndOfFile if buffer doesn't contain more data
     * @return ErrorCode_Success on success
     */
    [[nodiscard]] ErrorCode try_read (char* buf, size_t num_bytes_to_read,
                                      size_t& num_bytes_read) override;
    /**
     * Tries to seek from the beginning of the buffer to the given position
     * @param pos
     * @return ErrorCode_NotInit if the buffer is not initialized
     * @return ErrorCode_OutOfBounds if the given position > the buffer's size
     * @return ErrorCode_Success on success
     */
    [[nodiscard]] ErrorCode try_seek_from_begin (size_t pos) override;
    /**
     * Tries to get the current position of the read head in the buffer
     * @param pos Returns the position of the read head in the buffer
     * @return ErrorCode_NotInit if the buffer is not initialized
     * @return ErrorCode_Success on success
     */
    [[nodiscard]] ErrorCode try_get_pos (size_t& pos) override;

    /**
     * Sets the underlying buffer for this reader.
     * @param data
     * @param data_size
     **/
    void set_buffer (const char* data, size_t data_size) {
        m_data = data;
        m_data_size = data_size;
        m_cursor_pos = 0;
    }

private:
    const char* m_data;
    size_t m_data_size;
    size_t m_cursor_pos;
};

#endif // BufferReader_HPP
