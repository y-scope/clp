#ifndef BUFFERREADER_HPP
#define BUFFERREADER_HPP

// Project headers
#include "ReaderInterface.hpp"

/**
 * Class for reading from a fixed-size in-memory buffer
 */
class BufferReader : public ReaderInterface {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        [[nodiscard]] auto what() const noexcept -> char const* override {
            return "BufferReader operation failed";
        }
    };

    // Constructors
    BufferReader(char const* data, size_t data_size) : BufferReader(data, data_size, 0) {}

    BufferReader(char const* data, size_t data_size, size_t pos);

    // Methods implementing the ReaderInterface
    /**
     * Tries to read up to a given number of bytes from the buffer
     * @param buf
     * @param num_bytes_to_read The number of bytes to try and read
     * @param num_bytes_read The actual number of bytes read
     * @return ErrorCode_BadParam if buf is invalid
     * @return ErrorCode_EndOfFile if buffer doesn't contain more data
     * @return ErrorCode_Success on success
     */
    [[nodiscard]] auto try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read)
            -> ErrorCode override;
    /**
     * Tries to seek from the beginning of the buffer to the given position
     * @param pos
     * @return ErrorCode_OutOfBounds if the given position > the buffer's size
     * @return ErrorCode_Success on success
     */
    [[nodiscard]] auto try_seek_from_begin(size_t pos) -> ErrorCode override;
    /**
     * @param pos Returns the position of the read head in the buffer
     * @return ErrorCode_Success
     */
    [[nodiscard]] auto try_get_pos(size_t& pos) -> ErrorCode override;

    [[nodiscard]] auto
    try_read_to_delimiter(char delim, bool keep_delimiter, bool append, std::string& str)
            -> ErrorCode override;

    // Helper functions
    [[nodiscard]] auto get_buffer_size() const -> size_t { return m_internal_buf_size; }

    auto peek_buffer(char const*& buf, size_t& peek_size) -> void;

    auto try_read_to_delimiter(
            char delim,
            bool keep_delimiter,
            std::string& str,
            bool& found_delim,
            size_t& num_bytes_read
    ) -> ErrorCode;

private:
    // Method
    [[nodiscard]] auto get_remaining_data_size() const -> size_t {
        return m_internal_buf_size - m_internal_buf_pos;
    }

    // Variables
    char const* m_internal_buf;
    size_t m_internal_buf_size;
    size_t m_internal_buf_pos;
};

#endif  // BUFFERREADER_HPP
