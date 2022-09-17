#ifndef COMPRESSOR_FRONTEND_TOKEN_HPP
#define COMPRESSOR_FRONTEND_TOKEN_HPP

// C++ standard libraries
#include <string>
#include <vector>

namespace compressor_frontend {
    class Token {
    public:
        uint32_t start_pos;
        uint32_t end_pos;
        char** buffer_ptr;
        const uint32_t* buffer_size_ptr;
        uint32_t line;
        const std::vector<int>* type_ids;

        Token () : buffer_ptr(nullptr), buffer_size_ptr(nullptr), type_ids(nullptr), start_pos(0), end_pos(0), line(0) {}

        Token (uint32_t start_pos, uint32_t end_pos, char** buffer_ptr, const uint32_t* buffer_size_ptr, uint32_t line, const std::vector<int>* type_ids) :
                start_pos(start_pos), end_pos(end_pos), buffer_ptr(buffer_ptr), buffer_size_ptr(buffer_size_ptr), line(line), type_ids(type_ids) {}

        [[nodiscard]] std::string get_string () const;

        [[nodiscard]] std::string get_delimiter () const;

        [[nodiscard]] char get_char (uint8_t i) const;

        [[nodiscard]] uint32_t get_length () const;

        void set_string (int start_offset, int end_offset);
    };
}

#endif // COMPRESSOR_FRONTEND_TOKEN_HPP