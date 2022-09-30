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
#include "finite_automata/RegexDFA.hpp"
#include "finite_automata/RegexNFA.hpp"
#include "Token.hpp"

using compressor_frontend::finite_automata::RegexAST;
using compressor_frontend::finite_automata::RegexNFA;
using compressor_frontend::finite_automata::RegexDFA;

namespace compressor_frontend {
    template <typename NFAStateType, typename DFAStateType>
    class Lexer {
    public:
        // std::vector<int> can be declared as constexpr in c++20
        inline static const std::vector<int> cTokenEndTypes = {(int) SymbolID::TokenEndID};
        inline static const std::vector<int> cTokenUncaughtStringTypes = {(int) SymbolID::TokenUncaughtStringID};

        /**
         * A lexical rule has a name and regex pattern
         */
        struct Rule {
            // Constructor
            Rule (int n, std::unique_ptr<RegexAST<NFAStateType>> r) : m_name(n), m_regex(std::move(r)) {}

            /**
             * Adds AST representing the lexical rule to the NFA
             * @param nfa
             */
            void add_ast (RegexNFA<NFAStateType>* nfa) const;

            int m_name;
            std::unique_ptr<RegexAST<NFAStateType>> m_regex;
        };

        // Constructor
        Lexer () : m_byte_buf_pos(0), m_bytes_read(0), m_line(0), m_fail_pos(0), m_reduce_pos(0), m_match(false), m_match_pos(0), m_start_pos(0),
                   m_match_line(0), m_last_match_pos(0), m_last_match_line(0), m_type_ids(), m_is_delimiter(), m_is_first_char(), m_static_byte_buf(),
                   m_finished_reading_file(false), m_at_end_of_file(false), m_last_read_first_half_of_buf(false), m_reader(nullptr), m_has_delimiters(false),
                   m_active_byte_buf(nullptr), m_byte_buf_ptr(nullptr), m_byte_buf_size_ptr(nullptr), m_static_byte_buf_ptr(nullptr) {
            for (bool& i: m_is_first_char) {
                i = false;
            }
        }

        /**
         * Add a delimiters line from the schema to the lexer
         * @param delimiters
         */
        void add_delimiters (const std::vector<uint32_t>& delimiters);

        /**
         * Add lexical rule to the lexer's list of rules
         * @param id
         * @param regex
         */
        void add_rule (const uint32_t& id, std::unique_ptr<RegexAST<NFAStateType>> regex);

        /**
         * Return regex patter for a rule name
         * @param name
         * @return RegexAST*
         */
        RegexAST<NFAStateType>* get_rule (const uint32_t& name);

        /**
         * Generate DFA for lexer
         */
        void generate ();

        /**
         * Generate DFA for a reverse lexer matching the reverse of the words in the original language
         */
        void generate_reverse ();

        /**
         * Reset the lexer to start a new lexing (reset buffers, reset vars tracking positions)
         * @param reader
         */
        void reset (ReaderInterface& reader);

        /**
         * After lexing half of the buffer, reads into that half of the buffer and changes variables accordingly
         * @param next_children_start
         */
        void soft_reset (uint32_t& next_children_start);

        /**
         * Gets next token from the input string
         * If next token is an uncaught string, the next variable token is already prepped to be returned on the next call
         * @return Token
         */
        Token scan ();

        /**
         * scan(), but with wild wildcards in the input string (for search)
         * @param wildcard
         * @return Token
         */
        Token scan_with_wildcard (char wildcard);

        /**
         * Sets the position of where the last reduce was performed,
         * Used to know during lexing if half of the buffer has been lexed and needs to be read into
         * @param value
         */
        void set_reduce_pos (uint32_t value) {
            m_reduce_pos = value;
        }

        [[nodiscard]] const bool& get_has_delimiters() const {
            return m_has_delimiters;
        }

        [[nodiscard]] const bool& is_delimiter (uint8_t byte) const {
            return m_is_delimiter[byte];
        }

        // First character of any variable in the schema
        [[nodiscard]] const bool& is_first_char (uint8_t byte) const {
            return m_is_first_char[byte];
        }

        std::map<std::string, uint32_t> m_symbol_id;
        std::map<uint32_t, std::string> m_id_symbol;
        
    private:
        /**
         * Get next character from the input buffer
         * @return unsigned char
         */
        unsigned char get_next_character ();

        /**
         * Return epsilon_closure over m_epsilon_transitions
         * @return
         */
        std::set<NFAStateType*> epsilon_closure (NFAStateType* state_ptr);

        /**
        * Generate a DFA from the NFA
        * @param RegexNFA<NFAStateType> nfa
        * @return std::unique_ptr<RegexDFA<DFAStateType>>
        */
        unique_ptr<RegexDFA<DFAStateType>> nfa_to_dfa (RegexNFA<NFAStateType>& nfa);
        
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
        ReaderInterface* m_reader;
        bool m_has_delimiters;
        unique_ptr<RegexDFA<DFAStateType>> m_dfa;
    };

    namespace lexers {
        using ByteLexer = Lexer<finite_automata::RegexNFAByteState, finite_automata::RegexDFAByteState>;
        using UTF8Lexer = Lexer<finite_automata::RegexNFAUTF8State, finite_automata::RegexDFAUTF8State>;
    };
}

#include "Lexer.tpp"

#endif // COMPRESSOR_FRONTEND_LEXER_HPP
