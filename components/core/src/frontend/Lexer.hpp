#ifndef LEXER_HPP
#define LEXER_HPP

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_set>
#include <string>
#include <bitset>
#include <map>

#include "../ReaderInterface.hpp"
#include "FiniteAutomata.hpp"
#include "../Stopwatch.hpp"

namespace streaming_archive::writer {
    class File;
    class Archive;
}

class utf8 {
public:
    //0xC0, 0xC1, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF are invalid UTF-8 code units
    static const uint32_t ERROR = 0xFE;
    static const unsigned char CHAR_EOF = 0xFF;
};

class LALR1Parser;
class ParserAST;
class NonTerminal;
class Token;

class Lexer {
public:
    static Stopwatch get_codepoint_watch;
    static Stopwatch after_watch;

    uint32_t fail_pos;
    uint32_t m_reduce_pos;
    uint32_t m_match_pos;
    uint32_t m_start_pos;
    uint32_t m_match_line;
    uint32_t m_last_match_pos;
    uint32_t m_last_match_line;
    std::vector<int> const* m_type_ids;
    bool m_match = false;
    
    static const std::string TOKEN_END;
    static const std::string TOKEN_UNCAUGHT_STRING;
    static const int TOKEN_END_ID;
    static const int TOKEN_UNCAUGHT_STRING_ID;
    static const std::vector<int> TOKEN_END_TYPE;
    static const std::vector<int> TOKEN_UNCAUGHT_STRING_TYPE;
    static const uint32_t STATIC_BYTE_BUFF_SIZE = 60000;
    static uint32_t current_buff_size;
    void add_delimiters(const std::vector<uint32_t>& delimiters);
    void add_rule(uint32_t const& id, std::unique_ptr<RegexAST> regex);
    RegexAST* get_rule(uint32_t const& name);
    void generate();
    void generate_reverse();
    void reset(ReaderInterface* reader);
    void soft_reset();
    Lexer() : byte_buf_pos(0), bytes_read(0), line(0), fail_pos(0), m_reduce_pos(0), m_match_pos(0), m_start_pos(0), m_match_line(0), m_last_match_pos(0), 
              m_last_match_line(0), m_type_ids(), is_delimiter(), is_first_char(), archive_writer(nullptr), static_byte_buf(), 
              finished_reading_file(false), at_end_of_file(false), last_read_first_half_of_buf(false), reader(nullptr) {
        for (bool & i : is_first_char) {
            i = false; //RegexNFA::SIZE_OF_BYTE
        }
    }
    Token scan();
    Token scan_with_wildcard(char wildcard);

    struct Rule {
        int name;
        std::unique_ptr<RegexAST> regex;

        Rule(int n, std::unique_ptr<RegexAST> r) : name(n), regex(std::move(r)) { }
        void add_accepting_state(RegexNFA* nfa) const;
    };
    std::map<std::string, uint32_t> symbol_id;
    std::map<uint32_t, std::string> id_symbol;
    bool has_delimiters = false;
    bool is_delimiter[RegexNFA::SIZE_OF_BYTE];
    bool is_first_char[RegexNFA::SIZE_OF_BYTE];
    streaming_archive::writer::Archive* archive_writer;
    char* active_byte_buf = nullptr;
    char** byte_buf_ptr = nullptr;
    const uint32_t* byte_buf_size_ptr = nullptr;
    char* static_byte_buf_ptr = nullptr;
    char static_byte_buf[STATIC_BYTE_BUFF_SIZE];
    
private:
    bool finished_reading_file;
    bool at_end_of_file;
    std::vector<Rule> rules;
    uint32_t byte_buf_pos;
    bool last_read_first_half_of_buf;
    size_t bytes_read;
    uint32_t line;
    std::unique_ptr<RegexDFA> dfa;
    unsigned char get_next_character();
    ReaderInterface* reader;
};

class Token {
public:
    uint32_t start_pos;
    uint32_t end_pos;
    char** buffer_ptr;
    const uint32_t* buffer_size_ptr;
    uint32_t line;
    std::vector<int> const* type_ids;

    Token() : buffer_ptr(nullptr), buffer_size_ptr(nullptr), type_ids(nullptr), start_pos(0), end_pos(0), line(0) { }
    Token(uint32_t start_pos, uint32_t end_pos, char** buffer_ptr, const uint32_t* buffer_size_ptr, uint32_t line, std::vector<int> const* type_ids) :
            start_pos(start_pos), end_pos(end_pos), buffer_ptr(buffer_ptr), buffer_size_ptr(buffer_size_ptr), line(line), type_ids(type_ids) { }
    [[nodiscard]] std::string get_string() const;
    [[nodiscard]] std::string get_delimiter() const;
    [[nodiscard]] char get_char(uint8_t i) const;
    [[nodiscard]] uint32_t get_length() const;
    void set_string(int start_offset, int end_offset) {
        start_pos += start_offset;
        end_pos -= end_offset;
    }
};

#endif // LEXER_HPP
