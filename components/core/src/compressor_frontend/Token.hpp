#ifndef COMPRESSOR_FRONTEND_TOKEN_HPP
#define COMPRESSOR_FRONTEND_TOKEN_HPP

// C++ standard libraries
#include <string>
#include <vector>

namespace compressor_frontend {
    class Token {
    public:
        // Constructor
        Token () : m_buffer_ptr(nullptr), m_buffer_size_ptr(nullptr), m_type_ids(nullptr), m_start_pos(0), m_end_pos(0), m_line(0) {}

        // Constructor
        Token (uint32_t start_pos, uint32_t end_pos, char** buffer_ptr, const uint32_t* buffer_size_ptr, uint32_t line, const std::vector<int>* type_ids) :
                m_start_pos(start_pos), m_end_pos(end_pos), m_buffer_ptr(buffer_ptr), m_buffer_size_ptr(buffer_size_ptr), m_line(line), m_type_ids(type_ids) {}

        /**
         * Return the token string (string in the input buffer that the token represents)
         * @return std::string
         */
        [[nodiscard]] std::string get_string () const;

        /**
         * Return the first character (as a string) of the token string (which is a delimiter if delimiters are being used)
         * @return std::string
         */
        [[nodiscard]] std::string get_delimiter () const;

        /**
         * Return the ith character of the token string
         * @param i
         * @return char
         */
        [[nodiscard]] char get_char (uint8_t i) const;

        /**
         * Get the length of the token string
         * @return uint32_t
         */
        [[nodiscard]] uint32_t get_length () const;

        uint32_t m_start_pos;
        uint32_t m_end_pos;
        char** m_buffer_ptr;
        const uint32_t* m_buffer_size_ptr;
        uint32_t m_line;
        const std::vector<int>* m_type_ids;
    };
}

#endif // COMPRESSOR_FRONTEND_TOKEN_HPP