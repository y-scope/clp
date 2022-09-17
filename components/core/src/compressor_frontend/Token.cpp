#include "Token.hpp"

using std::string;

namespace compressor_frontend {

    string Token::get_string () const {
        if (start_pos <= end_pos) {
            return {*buffer_ptr + start_pos, *buffer_ptr + end_pos};
        } else {
            return string(*buffer_ptr + start_pos, *buffer_ptr + *buffer_size_ptr) +
                   string(*buffer_ptr, *buffer_ptr + end_pos);
        }
    }

    char Token::get_char (uint8_t i) const {
        return (*buffer_ptr)[start_pos + i];
    }

    string Token::get_delimiter () const {
        return {*buffer_ptr + start_pos, *buffer_ptr + start_pos + 1};
    }

    uint32_t Token::get_length () const {
        if (start_pos <= end_pos) {
            return end_pos - start_pos;
        } else {
            return *buffer_size_ptr - start_pos + end_pos;
        }
    }

    void Token::set_string (int start_offset, int end_offset) {
        start_pos += start_offset;
        end_pos -= end_offset;
    }
}