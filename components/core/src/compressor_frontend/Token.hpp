#ifndef COMPRESSOR_FRONTEND_TOKEN_HPP
#define COMPRESSOR_FRONTEND_TOKEN_HPP

// C++ standard libraries
#include <string>
#include <vector>

namespace compressor_frontend {
    class Token {
    public:
        Token () : m_buffer_ptr(nullptr), m_buffer_size_ptr(nullptr), m_type_ids(nullptr), m_start_pos(0), m_end_pos(0), m_line(0) {}

        Token (uint32_t start_pos, uint32_t end_pos, char** buffer_ptr, const uint32_t* buffer_size_ptr, uint32_t line, const std::vector<int>* type_ids) :
                m_start_pos(start_pos), m_end_pos(end_pos), m_buffer_ptr(buffer_ptr), m_buffer_size_ptr(buffer_size_ptr), m_line(line), m_type_ids(type_ids) {}

        [[nodiscard]] std::string get_string () const;

        [[nodiscard]] std::string get_delimiter () const;

        [[nodiscard]] char get_char (uint8_t i) const;

        [[nodiscard]] uint32_t get_length () const;

        void set_string (int start_offset, int end_offset);

        uint32_t m_start_pos;
        uint32_t m_end_pos;
        char** m_buffer_ptr;
        const uint32_t* m_buffer_size_ptr;
        uint32_t m_line;
        const std::vector<int>* m_type_ids;
    };
}

#endif // COMPRESSOR_FRONTEND_TOKEN_HPP