
#ifndef BufferReader_HPP
#define BufferReader_HPP

// C standard libraries

// C++ standard libraries

// Project headers
#include "ReaderInterface.hpp"

class BufferReader : public ReaderInterface {
public:
    BufferReader () : m_buffer(nullptr),
                      m_size(0),
                      m_cursor_pos(0),
                      m_checkpoint_pos(0) {}
    BufferReader (const int8_t* data, size_t size) :
            m_buffer(data),
            m_size(size),
            m_cursor_pos(0),
            m_checkpoint_pos(0) {}

    [[nodiscard]] ErrorCode try_read (char* buf, size_t num_bytes_to_read,
                                      size_t& num_bytes_read) override;
    [[nodiscard]] ErrorCode try_get_pos (size_t& pos) override;
    [[nodiscard]] ErrorCode try_seek_from_begin (size_t pos) override;

    void reset_buffer (const int8_t* data, size_t size) {
        m_buffer = data;
        m_size = size;
    }

    // The following methods should only be used by the decoder
    void mark_pos () { m_checkpoint_pos = m_cursor_pos; }
    void revert_pos () { m_cursor_pos = m_checkpoint_pos; }

    /**
     * Tries reading a string view of size = read_size from the ir_buf.
     * @param str_view Returns the string view
     * @param read_size
     * @return true on success, false if the ir_buf doesn't contain enough
     * data to decode
     **/
    [[nodiscard]] bool try_read_string_view (std::string_view& str_view, size_t read_size);

private:

    const int8_t* m_buffer;
    size_t m_size;
    size_t m_cursor_pos;
    size_t m_checkpoint_pos;
};


#endif // BufferReader_HPP
