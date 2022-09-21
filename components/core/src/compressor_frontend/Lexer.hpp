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
#include "finite_automata/RegexAST.hpp"
#include "Token.hpp"

namespace compressor_frontend {
    using finite_automata::RegexAST;
    using finite_automata::RegexNFA;
    using finite_automata::RegexDFA;
    
    class Lexer {
    public:
        // std::vector<int> can be declared as constexpr in c++20
        inline static const std::vector<int> cTokenEndTypes = {(int) SymbolID::TokenEndID};
        inline static const std::vector<int> cTokenUncaughtStringTypes = {(int) SymbolID::TokenUncaughtStringID};

        struct Rule {
            Rule (int n, std::unique_ptr<RegexAST> r) : m_name(n), m_regex(std::move(r)) {}

            void add_accepting_state (RegexNFA* nfa) const;

            int m_name;
            std::unique_ptr<RegexAST> m_regex;
        };

        Lexer () : m_byte_buf_pos(0), m_bytes_read(0), m_line(0), m_fail_pos(0), m_reduce_pos(0), m_match(false), m_match_pos(0), m_start_pos(0), m_match_line(0)
                   , m_last_match_pos(0), m_last_match_line(0), m_type_ids(), m_is_delimiter(), m_is_first_char(), m_static_byte_buf(), m_finished_reading_file(false)
                   , m_at_end_of_file(false), m_last_read_first_half_of_buf(false), m_reader(nullptr), m_has_delimiters(false), m_active_byte_buf(nullptr),
                m_byte_buf_ptr(nullptr), m_byte_buf_size_ptr(nullptr), m_static_byte_buf_ptr(nullptr) {
            for (bool& i: m_is_first_char) {
                i = false;
            }
        }
        
        void add_delimiters (const std::vector<uint32_t>& delimiters);

        void add_rule (const uint32_t& id, std::unique_ptr<finite_automata::RegexAST> regex);

        RegexAST* get_rule (const uint32_t& name);

        void generate ();

        void generate_reverse ();

        void reset (ReaderInterface& reader);

        void soft_reset ();

        Token scan ();

        Token scan_with_wildcard (char wildcard);

        void set_reduce_pos (uint32_t value) {
            m_reduce_pos = value;
        }
        
        [[nodiscard]] const bool& get_has_delimiters() const {
            return m_has_delimiters;
        }
        [[nodiscard]] const bool& is_delimiter (uint8_t byte) const {
            return m_is_delimiter[byte];
        }
        [[nodiscard]] const bool& is_first_char (uint8_t byte) const {
            return m_is_first_char[byte];
        }

        std::map<std::string, uint32_t> m_symbol_id;
        std::map<uint32_t, std::string> m_id_symbol;
        
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
        static uint32_t m_current_buff_size;
        bool m_is_delimiter[cSizeOfByte];
        bool m_is_first_char[cSizeOfByte];
        char* m_active_byte_buf;
        char** m_byte_buf_ptr;
        const uint32_t* m_byte_buf_size_ptr;
        char* m_static_byte_buf_ptr;
        char m_static_byte_buf[cStaticByteBuffSize];
        bool m_finished_reading_file;
        bool m_at_end_of_file;
        std::vector<Rule> m_rules;
        uint32_t m_byte_buf_pos;
        bool m_last_read_first_half_of_buf;
        size_t m_bytes_read;
        uint32_t m_line;
        /// TODO: make this not a unique_ptr and test performance difference
        std::unique_ptr<RegexDFA> m_dfa;
        ReaderInterface* m_reader;
        bool m_has_delimiters;
    };
}

#endif // COMPRESSOR_FRONTEND_LEXER_HPP
