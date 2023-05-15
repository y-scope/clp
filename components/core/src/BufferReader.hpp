
#ifndef BufferReader_HPP
#define BufferReader_HPP

// C standard libraries

// C++ standard libraries

// Project headers
#include "ReaderInterface.hpp"

class BufferReader : public ReaderInterface {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) :
            TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "BufferReader operation failed";
        }
    };
    BufferReader () : m_data(nullptr),
                      m_size(0),
                      m_cursor_pos(0) {}
    BufferReader (const int8_t* data, size_t size) :
            m_data(data),
            m_size(size),
            m_cursor_pos(0) {}

    [[nodiscard]] ErrorCode try_read (char* buf, size_t num_bytes_to_read,
                                      size_t& num_bytes_read) override;
    [[nodiscard]] ErrorCode try_get_pos (size_t& pos) override;
    [[nodiscard]] ErrorCode try_seek_from_begin (size_t pos) override;
    [[nodiscard]] size_t get_buffer_length() { return m_size; }
    void reset_buffer (const int8_t* data, size_t size) {
        m_data = data;
        m_size = size;
        m_cursor_pos = 0;
    }

    /**
     * Tries reading a string view of size = read_size from the ir_buf.
     * @param str_view Returns the string view
     * @param read_size
     * @return true on success, false if the ir_buf doesn't contain enough
     * data to decode
     **/
    [[nodiscard]] bool try_read_string_view (std::string_view& str_view, size_t read_size);

protected:
    void reset_buffer (const int8_t* data, size_t size, size_t cursor_pos) {
        m_data = data;
        m_size = size;
        m_cursor_pos = cursor_pos;
    }
    const int8_t* m_data;
    size_t m_size;
    size_t m_cursor_pos;
};


#endif // BufferReader_HPP
