#include "Token.hpp"

using std::string;

namespace compressor_frontend {

    string Token::get_string () const {
        if (m_start_pos <= m_end_pos) {
            return {*m_buffer_ptr + m_start_pos, *m_buffer_ptr + m_end_pos};
        } else {
            return string(*m_buffer_ptr + m_start_pos, *m_buffer_ptr + *m_buffer_size_ptr) +
                   string(*m_buffer_ptr, *m_buffer_ptr + m_end_pos);
        }
    }

    char Token::get_char (uint8_t i) const {
        return (*m_buffer_ptr)[m_start_pos + i];
    }

    string Token::get_delimiter () const {
        return {*m_buffer_ptr + m_start_pos, *m_buffer_ptr + m_start_pos + 1};
    }

    uint32_t Token::get_length () const {
        if (m_start_pos <= m_end_pos) {
            return m_end_pos - m_start_pos;
        } else {
            return *m_buffer_size_ptr - m_start_pos + m_end_pos;
        }
    }
}