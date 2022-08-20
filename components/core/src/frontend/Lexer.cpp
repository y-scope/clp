#include <cassert>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

#include "FiniteAutomata.hpp"
#include "Lexer.hpp"
#include "LALR1Parser.hpp"
#include "../streaming_archive/writer/Archive.hpp"

/**
 * utf8 format (https://en.wikipedia.org/wiki/UTF-8)
 * 1 byte: 0x0 - 0x80 : 0xxxxxxx
 * 2 byte: 0x80 - 0x7FF : 110xxxxx 10xxxxxx
 * 3 byte: 0x800 - 0xFFFF : 1110xxxx 10xxxxxx 10xxxxxx
 * 4 byte: 0x10000 - 0x1FFFFF : 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 */

const std::string Lexer::TOKEN_END = "$end";
const std::string Lexer::TOKEN_UNCAUGHT_STRING = "$UncaughtString";
const int Lexer::TOKEN_END_ID = 0;
const int Lexer::TOKEN_UNCAUGHT_STRING_ID = 1;
const std::vector<int> Lexer::TOKEN_END_TYPE = { Lexer::TOKEN_END_ID };
const std::vector<int> Lexer::TOKEN_UNCAUGHT_STRING_TYPE = {Lexer::TOKEN_UNCAUGHT_STRING_ID };
const uint32_t Lexer::STATIC_BYTE_BUFF_SIZE;
uint32_t Lexer::current_buff_size;

std::string Token::get_string() const {
    if (start_pos <= end_pos) {
        return {*buffer_ptr + start_pos, *buffer_ptr + end_pos};
    } else {
        return std::string(*buffer_ptr + start_pos, *buffer_ptr + *buffer_size_ptr) +
               std::string(*buffer_ptr, *buffer_ptr + end_pos);
    }
}

char Token::get_char(uint8_t i) const {
    return (*buffer_ptr)[start_pos + i];
}

std::string Token::get_delimiter() const {
    return {*buffer_ptr + start_pos, *buffer_ptr + start_pos + 1};
}

uint32_t Token::get_length() const {
    if (start_pos <= end_pos) {
        return end_pos - start_pos;
    } else {
        return *buffer_size_ptr - start_pos  + end_pos;
    }
}

void Lexer::soft_reset() {
    if (NonTerminal::next_children_start > LALR1Parser::SIZE_OF_ALL_CHILDREN/2) {
        NonTerminal::next_children_start = 0;
    }
    if (finished_reading_file) {
        return;
    }
    if (m_reduce_pos == -1) {
        m_reduce_pos += current_buff_size;
    }
    if ((!last_read_first_half_of_buf && m_reduce_pos > current_buff_size/2) ||
        (last_read_first_half_of_buf && m_reduce_pos < current_buff_size/2 && m_reduce_pos > 0)) { 
        uint32_t offset = 0;
        if (last_read_first_half_of_buf) {
            offset = current_buff_size / 2;
        }
        reader->read(active_byte_buf + offset, current_buff_size / 2, bytes_read);

        if (bytes_read < current_buff_size / 2) {
            finished_reading_file = true;
        }
        last_read_first_half_of_buf = !last_read_first_half_of_buf;
        bytes_read += offset;
        if (m_reduce_pos >= current_buff_size/2) {
            fail_pos = current_buff_size/2;
        } else {
            fail_pos = 0;
        }
    }
}

unsigned char Lexer::get_next_character() {
    if (finished_reading_file && byte_buf_pos == bytes_read) {
        at_end_of_file = true;
        return utf8::CHAR_EOF;
    }
    unsigned char character = active_byte_buf[byte_buf_pos];
    byte_buf_pos++;
    if (byte_buf_pos == current_buff_size) {
        byte_buf_pos = 0;
    }
    return character;
}

Token Lexer::scan() {
    if (m_match) {
        m_match = false;
        m_last_match_pos = m_match_pos;
        m_last_match_line = m_match_line;
        return Token { m_start_pos, m_match_pos, byte_buf_ptr, byte_buf_size_ptr, m_match_line, m_type_ids };
    }
    m_start_pos = byte_buf_pos; 
    m_match_pos = byte_buf_pos;
    m_match_line = line;
    m_type_ids = nullptr;
    RegexDFA::State* state = dfa->root();
    while (true) {
        if (byte_buf_pos == fail_pos) {    
            std::string warn = "Long line detected";
            warn += " at line " + std::to_string(line);
            warn += " in file " + dynamic_cast<FileReader*>(reader)->get_path();
            warn += " changing to dynamic buffer and increasing buffer size to ";
            warn += std::to_string(current_buff_size * 2);
            SPDLOG_WARN("{}", warn.c_str());
            // Found a super long line: for completeness handle this case, but efficiency doesn't matter
            // 1. copy everything from old buffer into new buffer
            if (active_byte_buf == static_byte_buf) {
                active_byte_buf = (char*) malloc(current_buff_size * sizeof(char));
                if (fail_pos == 0) {
                    memcpy(active_byte_buf, static_byte_buf, sizeof(static_byte_buf));
                } else {
                    /// TODO: make a test case for this scenario
                    memcpy(active_byte_buf, static_byte_buf + sizeof(static_byte_buf) / 2, sizeof(static_byte_buf) / 2);
                    memcpy(active_byte_buf + sizeof(static_byte_buf) / 2, static_byte_buf, sizeof(static_byte_buf) / 2);
                    if (m_match_pos >= current_buff_size / 2) {
                        m_match_pos -= current_buff_size / 2;
                    } else {
                        m_match_pos += current_buff_size / 2;
                    }
                    if (m_start_pos >= current_buff_size / 2) {
                        m_start_pos -= current_buff_size / 2;
                    } else {
                        m_start_pos += current_buff_size / 2;
                    }
                    if (m_last_match_pos >= current_buff_size / 2) {
                        m_last_match_pos -= current_buff_size / 2;
                    } else {
                        m_last_match_pos += current_buff_size / 2;
                    }
                }
            }
            current_buff_size *= 2;
            active_byte_buf = (char*) realloc(active_byte_buf, current_buff_size * sizeof(char));
            byte_buf_ptr = &active_byte_buf;
            byte_buf_size_ptr = &current_buff_size;
            if (active_byte_buf == nullptr) {
                SPDLOG_ERROR("failed to allocate byte buffer of size {}", current_buff_size);
                std::string err = "Lexer failed to find a match after checking entire buffer";
                err += " at line " + std::to_string(line);
                err += " in file " + dynamic_cast<FileReader*>(reader)->get_path();
                dynamic_cast<FileReader*>(reader)->close();
                throw(err); // this throw allows for continuation of compressing other files 
            }
            reader->read(active_byte_buf + current_buff_size / 2, current_buff_size / 2, bytes_read);
            bytes_read += current_buff_size / 2;
            if (bytes_read < current_buff_size) {
                finished_reading_file = true;
            }
            byte_buf_pos = current_buff_size / 2;
            fail_pos =  0;
        }
        uint32_t prev_byte_buf_pos = byte_buf_pos;
        unsigned char next_char = get_next_character();
        if ((is_delimiter[next_char] || at_end_of_file || !has_delimiters) && !state->terminals.empty()) {
            m_match = true;
            m_type_ids = &(state->terminals);
            m_match_pos = prev_byte_buf_pos;
            m_match_line = line;
        }
        RegexDFA::State* next = state->byte_transitions[next_char];
        if (next_char == '\n') {
            line++;
            if (has_delimiters && !m_match) {
                next = dfa->root()->byte_transitions[next_char];
                m_match = true;
                m_type_ids = &(next->terminals);
                m_start_pos = prev_byte_buf_pos;
                m_match_pos = byte_buf_pos;
                m_match_line = line;
            }
        }
        if (at_end_of_file || next == nullptr) {
            if (m_match) {
                at_end_of_file = false;
                byte_buf_pos = m_match_pos;
                line = m_match_line;
                if (m_last_match_pos != m_start_pos) {
                    return Token{ m_last_match_pos, m_start_pos, byte_buf_ptr, byte_buf_size_ptr, m_last_match_line, &TOKEN_UNCAUGHT_STRING_TYPE };
                }
                m_match = false;
                m_last_match_pos = m_match_pos;
                m_last_match_line = m_match_line;
                return Token{ m_start_pos, m_match_pos, byte_buf_ptr, byte_buf_size_ptr, m_match_line, m_type_ids };
            } else if (at_end_of_file && m_start_pos == byte_buf_pos) {
                if (m_last_match_pos != m_start_pos) {
                    m_match_pos = byte_buf_pos;
                    m_type_ids = &TOKEN_END_TYPE;
                    m_match = true;
                    return Token{ m_last_match_pos, m_start_pos, byte_buf_ptr, byte_buf_size_ptr, m_last_match_line, &TOKEN_UNCAUGHT_STRING_TYPE };
                }
                return Token{ byte_buf_pos, byte_buf_pos, byte_buf_ptr, byte_buf_size_ptr, line, &TOKEN_END_TYPE };
            } else {
                while (!at_end_of_file && !is_first_char[next_char]) {
                    prev_byte_buf_pos = byte_buf_pos;
                    next_char = get_next_character();
                }
                byte_buf_pos = prev_byte_buf_pos;
                m_start_pos = prev_byte_buf_pos;
                state = dfa->root();
                continue;
            }
        }
        state = next;
    }
}

Token Lexer::scan_with_wildcard(char wildcard) {
    if (m_match) {
        m_match = false;
        m_last_match_pos = m_match_pos;
        m_last_match_line = m_match_line;
        return Token { m_start_pos, m_match_pos, byte_buf_ptr, byte_buf_size_ptr, m_match_line, m_type_ids };
    }
    m_start_pos = byte_buf_pos;
    m_match_pos = byte_buf_pos;
    m_match_line = line;
    m_type_ids = nullptr;
    RegexDFA::State* state = dfa->root();
    while (true) {
        if (byte_buf_pos == fail_pos) {
            std::string warn = "Long line detected";
            warn += " at line " + std::to_string(line);
            warn += " in file " + dynamic_cast<FileReader*>(reader)->get_path();
            warn += " changing to dynamic buffer and increasing buffer size to ";
            warn += std::to_string(current_buff_size * 2);
            SPDLOG_WARN("{}", warn.c_str());
            // Found a super long line: for completeness handle this case, but efficiency doesn't matter
            // 1. copy everything from old buffer into new buffer
            if (active_byte_buf == static_byte_buf) {
                active_byte_buf = (char*) malloc(current_buff_size * sizeof(char));
                if (fail_pos == 0) {
                    memcpy(active_byte_buf, static_byte_buf, sizeof(static_byte_buf));
                } else {
                    /// TODO: make a test case for this scenario
                    memcpy(active_byte_buf, static_byte_buf + sizeof(static_byte_buf) / 2, sizeof(static_byte_buf) / 2);
                    memcpy(active_byte_buf + sizeof(static_byte_buf) / 2, static_byte_buf, sizeof(static_byte_buf) / 2);
                    if (m_match_pos >= current_buff_size / 2) {
                        m_match_pos -= current_buff_size / 2;
                    } else {
                        m_match_pos += current_buff_size / 2;
                    }
                    if (m_start_pos >= current_buff_size / 2) {
                        m_start_pos -= current_buff_size / 2;
                    } else {
                        m_start_pos += current_buff_size / 2;
                    }
                    if (m_last_match_pos >= current_buff_size / 2) {
                        m_last_match_pos -= current_buff_size / 2;
                    } else {
                        m_last_match_pos += current_buff_size / 2;
                    }
                }
            }
            current_buff_size *= 2;
            active_byte_buf = (char*) realloc(active_byte_buf, current_buff_size * sizeof(char));
            byte_buf_ptr = &active_byte_buf;
            byte_buf_size_ptr = &current_buff_size;
            if (active_byte_buf == nullptr) {
                SPDLOG_ERROR("failed to allocate byte buffer of size {}", current_buff_size);
                std::string err = "Lexer failed to find a match after checking entire buffer";
                err += " at line " + std::to_string(line);
                err += " in file " + dynamic_cast<FileReader*>(reader)->get_path();
                dynamic_cast<FileReader*>(reader)->close();
                throw(err); // this throw allows for continuation of compressing other files 
            }
            reader->read(active_byte_buf + current_buff_size / 2, current_buff_size / 2, bytes_read);
            bytes_read += current_buff_size / 2;
            if (bytes_read < current_buff_size) {
                finished_reading_file = true;
            }
            byte_buf_pos = current_buff_size / 2;
            fail_pos =  0;
        }
        uint32_t prev_byte_buf_pos = byte_buf_pos;
        unsigned char next_char = get_next_character();
        if ((is_delimiter[next_char] || at_end_of_file || !has_delimiters) && !state->terminals.empty()) {
            m_match = true;
            m_type_ids = &(state->terminals);
            m_match_pos = prev_byte_buf_pos;
            m_match_line = line;
        }
        RegexDFA::State* next = state->byte_transitions[next_char];
        if (next_char == '\n') {
            line++;
            if (has_delimiters && !m_match) {
                next = dfa->root()->byte_transitions[next_char];
                m_match = true;
                m_type_ids = &(next->terminals);
                m_start_pos = prev_byte_buf_pos;
                m_match_pos = byte_buf_pos;
                m_match_line = line;
            }
        }
        
        // !at_end_of_file should be impossible
        // m_match_pos != byte_buf_pos --> "te matches from "tes*" (means "tes" isn't a match, so is_var = false)
        // 
        if (at_end_of_file || next == nullptr) {
            assert(at_end_of_file);
            
            if (!m_match || (m_match && m_match_pos != byte_buf_pos)) {
                return Token{ m_last_match_pos, byte_buf_pos, byte_buf_ptr, byte_buf_size_ptr, m_last_match_line, &TOKEN_UNCAUGHT_STRING_TYPE };
            }
            if (m_match) {
                // BFS (keep track of m_type_ids)
                if (wildcard == '?') {
                    for (auto next_state : state->byte_transitions) {
                        if (next_state->terminals.empty()) {
                            return Token{ m_last_match_pos, byte_buf_pos, byte_buf_ptr, byte_buf_size_ptr, m_last_match_line, &TOKEN_UNCAUGHT_STRING_TYPE };
                        }                         
                    }
                } else if (wildcard == '*') {
                    std::stack<RegexDFA::State*> unvisited_states;
                    std::set<RegexDFA::State*> visited_states;
                    unvisited_states.push(state);
                    while (!unvisited_states.empty()) {
                        RegexDFA::State* current_state = unvisited_states.top();
                        if (current_state == nullptr || current_state->terminals.empty()) {
                            return Token{ m_last_match_pos, byte_buf_pos, byte_buf_ptr, byte_buf_size_ptr, m_last_match_line, &TOKEN_UNCAUGHT_STRING_TYPE };
                        }
                        unvisited_states.pop();
                        visited_states.insert(current_state);
                        for (uint32_t byte = 0; byte < RegexNFA::SIZE_OF_BYTE; byte++) {
                            if (is_delimiter[byte]) {
                                continue;
                            }
                            RegexDFA::State* next_state = current_state->byte_transitions[byte];
                            if (visited_states.find(next_state) == visited_states.end()) {
                                unvisited_states.push(next_state);
                            }
                        }
                    }
                }
                byte_buf_pos = m_match_pos;
                line = m_match_line;
                m_match = false;
                m_last_match_pos = m_match_pos;
                m_last_match_line = m_match_line;
                return Token{ m_start_pos, m_match_pos, byte_buf_ptr, byte_buf_size_ptr, m_match_line, m_type_ids };
            }
        }
        state = next;
    }
}

// If reset() is called all Tokens previously created by the lexer are invalid
void Lexer::reset(ReaderInterface* reader_interface) {
    reader = reader_interface;
    finished_reading_file = false;
    at_end_of_file = false;
    m_reduce_pos = 0;
    m_last_match_pos = 0;
    m_match = false;
    byte_buf_pos = 0;
    line = 0;
    //if (bytes_read != 0) {
    //    delete active_byte_buf;
    //}
    bytes_read = 0;
    last_read_first_half_of_buf = true;
    if (active_byte_buf != nullptr && active_byte_buf != static_byte_buf) {
        free(active_byte_buf);
    }
    static_byte_buf_ptr = static_byte_buf;
    active_byte_buf = static_byte_buf;
    current_buff_size = STATIC_BYTE_BUFF_SIZE;
    byte_buf_ptr = &static_byte_buf_ptr;
    byte_buf_size_ptr = &STATIC_BYTE_BUFF_SIZE;
    
    reader->read(active_byte_buf, current_buff_size / 2, bytes_read);
    if (bytes_read < current_buff_size / 2) {
        finished_reading_file = true;
    }
    fail_pos =  current_buff_size/2;
    m_match_pos = 0;
    m_start_pos = 0;
    m_match_line = 0;
    m_last_match_line = 0;
    m_type_ids = nullptr;
}

void Lexer::add_delimiters(const std::vector<uint32_t>& delimiters) {
    assert(!delimiters.empty());
    has_delimiters = true;
    for (bool & i : is_delimiter) {
        i = false;
    }
    for (uint32_t delimiter : delimiters) {
        is_delimiter[delimiter] = true;
    }
}

void Lexer::add_rule(uint32_t const& id, std::unique_ptr<RegexAST> rule) {
    rules.emplace_back(id, std::move(rule));
}

RegexAST* Lexer::get_rule(uint32_t const& name) {
    for (Rule& rule : rules) {
        if (rule.name == name) {
            return rule.regex.get();
        }
    }
    return nullptr;
}

void Lexer::generate() {
    RegexNFA nfa;
    for (Rule const& r : rules) {
        r.add_accepting_state(&nfa);
    }
    dfa = nfa.to_dfa();
    
    RegexDFA::State* state = dfa->root();
    for (uint32_t i = 0; i < RegexNFA::SIZE_OF_BYTE; i++) {
        if (state->byte_transitions[i] != nullptr) {
            is_first_char[i] = true;
        } else {
            is_first_char[i] = false;
        }
    }
}

void Lexer::generate_reverse() {
    RegexNFA nfa;
    for (Rule const& r : rules) {
        r.add_accepting_state(&nfa);
    }
    nfa.reverse();
    dfa = nfa.to_dfa();

    RegexDFA::State* state = dfa->root();
    for (uint32_t i = 0; i < RegexNFA::SIZE_OF_BYTE; i++) {
        if (state->byte_transitions[i] != nullptr) {
            is_first_char[i] = true;
        } else {
            is_first_char[i] = false;
        }
    }
}

void Lexer::Rule::add_accepting_state(RegexNFA* nfa) const {
    RegexNFA::State* s = nfa->new_state();
    s->accepting = true;
    s->tag = name;
    regex->add_state(nfa, s);
}
