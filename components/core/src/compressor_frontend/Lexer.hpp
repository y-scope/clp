#ifndef COMPRESSOR_FRONTEND_LEXER_HPP
#define COMPRESSOR_FRONTEND_LEXER_HPP

// C++ standard libraries
#include <bitset>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

// Project headers
#include "../ReaderInterface.hpp"
#include "../Stopwatch.hpp"
#include "Constants.hpp"
#include "FiniteAutomata.hpp"
#include "Token.hpp"

namespace compressor_frontend {
    class Lexer {
    public:
        // std::vector<int> can be declared as constexpr in c++20
        inline static const std::vector<int> cTokenEndTypes = {(int) SymbolID::TokenEndID};
        inline static const std::vector<int> cTokenUncaughtStringTypes = {(int) SymbolID::TokenUncaughtStringID};
        
        void add_delimiters (const std::vector<uint32_t>& delimiters);

        void add_rule (const uint32_t& id, std::unique_ptr<RegexAST> regex);

        RegexAST* get_rule (const uint32_t& name);

        void generate ();

        void generate_reverse ();

        void reset (ReaderInterface& reader);

        void soft_reset ();

        Lexer () : byte_buf_pos(0), bytes_read(0), line(0), m_fail_pos(0), m_reduce_pos(0), m_match(false), m_match_pos(0), m_start_pos(0), m_match_line(0)
                   , m_last_match_pos(0), m_last_match_line(0), m_type_ids(), is_delimiter(), is_first_char(), static_byte_buf(), finished_reading_file(false)
                   , at_end_of_file(false), last_read_first_half_of_buf(false), reader(nullptr), has_delimiters(false), active_byte_buf(nullptr), 
                   byte_buf_ptr(nullptr), byte_buf_size_ptr(nullptr), static_byte_buf_ptr(nullptr) {
            for (bool& i: is_first_char) {
                i = false;
            }
        }

        Token scan ();

        Token scan_with_wildcard (char wildcard);

        struct Rule {
            int name;
            std::unique_ptr<RegexAST> regex;

            Rule (int n, std::unique_ptr<RegexAST> r) : name(n), regex(std::move(r)) {}

            void add_accepting_state (RegexNFA* nfa) const;
        };

        void set_reduce_pos (uint32_t value) {
            m_reduce_pos = value;
        }

        std::map<std::string, uint32_t> symbol_id;
        std::map<uint32_t, std::string> id_symbol;
        
        [[nodiscard]] const bool& get_has_delimiters() const {
            return has_delimiters;
        }
        [[nodiscard]] const bool& check_is_delimiter (uint8_t byte) const {
            return is_delimiter[byte];
        }
        [[nodiscard]] const bool& check_is_first_char (uint8_t byte) const {
            return is_first_char[byte];
        }
        
    private:
        unsigned char get_next_character ();
        
        uint32_t m_fail_pos;
        uint32_t m_reduce_pos;
        uint32_t m_match_pos;
        uint32_t m_start_pos;
        uint32_t m_match_line;
        uint32_t m_last_match_pos;
        uint32_t m_last_match_line;
        bool m_match;
        const std::vector<int>* m_type_ids;
        static uint32_t current_buff_size;
        bool is_delimiter[cSizeOfByte];
        bool is_first_char[cSizeOfByte];
        char* active_byte_buf;
        char** byte_buf_ptr;
        const uint32_t* byte_buf_size_ptr;
        char* static_byte_buf_ptr;
        char static_byte_buf[cStaticByteBuffSize];
        bool finished_reading_file;
        bool at_end_of_file;
        std::vector<Rule> rules;
        uint32_t byte_buf_pos;
        bool last_read_first_half_of_buf;
        size_t bytes_read;
        uint32_t line;
        std::unique_ptr<RegexDFA> dfa;
        ReaderInterface* reader;
        bool has_delimiters;
    };
}

#endif // COMPRESSOR_FRONTEND_LEXER_HPP
