#include "Lexer.hpp"

// C++ standard libraries
#include <cassert>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

// Project headers
#include "../streaming_archive/writer/Archive.hpp"
#include "Constants.hpp"
#include "FiniteAutomata.hpp"
#include "LALR1Parser.hpp"

using std::string;
using std::to_string;

/**
 * utf8 format (https://en.wikipedia.org/wiki/UTF-8)
 * 1 byte: 0x0 - 0x80 : 0xxxxxxx
 * 2 byte: 0x80 - 0x7FF : 110xxxxx 10xxxxxx
 * 3 byte: 0x800 - 0xFFFF : 1110xxxx 10xxxxxx 10xxxxxx
 * 4 byte: 0x10000 - 0x1FFFFF : 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 */
namespace compressor_frontend {
    uint32_t Lexer::current_buff_size;

    void Lexer::soft_reset () {
        if (NonTerminal::next_children_start > cSizeOfAllChildren / 2) {
            NonTerminal::next_children_start = 0;
        }
        if (finished_reading_file) {
            return;
        }
        if (m_reduce_pos == -1) {
            m_reduce_pos += current_buff_size;
        }
        if ((!last_read_first_half_of_buf && m_reduce_pos > current_buff_size / 2) ||
            (last_read_first_half_of_buf && m_reduce_pos < current_buff_size / 2 && m_reduce_pos > 0)) {
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
            if (m_reduce_pos >= current_buff_size / 2) {
                m_fail_pos = current_buff_size / 2;
            } else {
                m_fail_pos = 0;
            }
        }
    }

    unsigned char Lexer::get_next_character () {
        if (finished_reading_file && byte_buf_pos == bytes_read) {
            at_end_of_file = true;
            return utf8::cCharEOF;
        }
        unsigned char character = active_byte_buf[byte_buf_pos];
        byte_buf_pos++;
        if (byte_buf_pos == current_buff_size) {
            byte_buf_pos = 0;
        }
        return character;
    }

    Token Lexer::scan () {
        if (m_match) {
            m_match = false;
            m_last_match_pos = m_match_pos;
            m_last_match_line = m_match_line;
            return Token{m_start_pos, m_match_pos, byte_buf_ptr, byte_buf_size_ptr, m_match_line, m_type_ids};
        }
        m_start_pos = byte_buf_pos;
        m_match_pos = byte_buf_pos;
        m_match_line = line;
        m_type_ids = nullptr;
        RegexDFA::State* state = dfa->root();
        while (true) {
            if (byte_buf_pos == m_fail_pos) {
                string warn = "Long line detected";
                warn += " at line " + to_string(line);
                warn += " in file " + dynamic_cast<FileReader*>(reader)->get_path();
                warn += " changing to dynamic buffer and increasing buffer size to ";
                warn += to_string(current_buff_size * 2);
                SPDLOG_WARN(warn);
                // Found a super long line: for completeness handle this case, but efficiency doesn't matter
                // 1. copy everything from old buffer into new buffer
                if (active_byte_buf == static_byte_buf) {
                    active_byte_buf = (char*) malloc(current_buff_size * sizeof(char));
                    if (m_fail_pos == 0) {
                        memcpy(active_byte_buf, static_byte_buf, sizeof(static_byte_buf));
                    } else {
                        //TODO: make a test case for this scenario
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
                    string err = "Lexer failed to find a match after checking entire buffer";
                    err += " at line " + to_string(line);
                    err += " in file " + dynamic_cast<FileReader*>(reader)->get_path();
                    dynamic_cast<FileReader*>(reader)->close();
                    throw (err); // this throw allows for continuation of compressing other files 
                }
                reader->read(active_byte_buf + current_buff_size / 2, current_buff_size / 2, bytes_read);
                bytes_read += current_buff_size / 2;
                if (bytes_read < current_buff_size) {
                    finished_reading_file = true;
                }
                byte_buf_pos = current_buff_size / 2;
                m_fail_pos = 0;
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
                        return Token{m_last_match_pos, m_start_pos, byte_buf_ptr, byte_buf_size_ptr, m_last_match_line, &cTokenUncaughtStringTypes};
                    }
                    m_match = false;
                    m_last_match_pos = m_match_pos;
                    m_last_match_line = m_match_line;
                    return Token{m_start_pos, m_match_pos, byte_buf_ptr, byte_buf_size_ptr, m_match_line, m_type_ids};
                } else if (at_end_of_file && m_start_pos == byte_buf_pos) {
                    if (m_last_match_pos != m_start_pos) {
                        m_match_pos = byte_buf_pos;
                        m_type_ids = &cTokenEndTypes;
                        m_match = true;
                        return Token{m_last_match_pos, m_start_pos, byte_buf_ptr, byte_buf_size_ptr, m_last_match_line, &cTokenUncaughtStringTypes};
                    }
                    return Token{byte_buf_pos, byte_buf_pos, byte_buf_ptr, byte_buf_size_ptr, line, &cTokenEndTypes};
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

    Token Lexer::scan_with_wildcard (char wildcard) {
        if (m_match) {
            m_match = false;
            m_last_match_pos = m_match_pos;
            m_last_match_line = m_match_line;
            return Token{m_start_pos, m_match_pos, byte_buf_ptr, byte_buf_size_ptr, m_match_line, m_type_ids};
        }
        m_start_pos = byte_buf_pos;
        m_match_pos = byte_buf_pos;
        m_match_line = line;
        m_type_ids = nullptr;
        RegexDFA::State* state = dfa->root();
        while (true) {
            if (byte_buf_pos == m_fail_pos) {
                string warn = "Long line detected";
                warn += " at line " + to_string(line);
                warn += " in file " + dynamic_cast<FileReader*>(reader)->get_path();
                warn += " changing to dynamic buffer and increasing buffer size to ";
                warn += to_string(current_buff_size * 2);
                SPDLOG_WARN(warn);
                // Found a super long line: for completeness handle this case, but efficiency doesn't matter
                // 1. copy everything from old buffer into new buffer
                if (active_byte_buf == static_byte_buf) {
                    active_byte_buf = (char*) malloc(current_buff_size * sizeof(char));
                    if (m_fail_pos == 0) {
                        memcpy(active_byte_buf, static_byte_buf, sizeof(static_byte_buf));
                    } else {
                        // TODO: make a test case for this scenario
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
                    string err = "Lexer failed to find a match after checking entire buffer";
                    err += " at line " + to_string(line);
                    err += " in file " + dynamic_cast<FileReader*>(reader)->get_path();
                    dynamic_cast<FileReader*>(reader)->close();
                    throw (err); // this throw allows for continuation of compressing other files 
                }
                reader->read(active_byte_buf + current_buff_size / 2, current_buff_size / 2, bytes_read);
                bytes_read += current_buff_size / 2;
                if (bytes_read < current_buff_size) {
                    finished_reading_file = true;
                }
                byte_buf_pos = current_buff_size / 2;
                m_fail_pos = 0;
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
                    return Token{m_last_match_pos, byte_buf_pos, byte_buf_ptr, byte_buf_size_ptr, m_last_match_line, &cTokenUncaughtStringTypes};
                }
                if (m_match) {
                    // BFS (keep track of m_type_ids)
                    if (wildcard == '?') {
                        for (auto next_state: state->byte_transitions) {
                            if (next_state->terminals.empty()) {
                                return Token{m_last_match_pos, byte_buf_pos, byte_buf_ptr, byte_buf_size_ptr, m_last_match_line, &cTokenUncaughtStringTypes};
                            }
                        }
                    } else if (wildcard == '*') {
                        std::stack<RegexDFA::State*> unvisited_states;
                        std::set<RegexDFA::State*> visited_states;
                        unvisited_states.push(state);
                        while (!unvisited_states.empty()) {
                            RegexDFA::State* current_state = unvisited_states.top();
                            if (current_state == nullptr || current_state->terminals.empty()) {
                                return Token{m_last_match_pos, byte_buf_pos, byte_buf_ptr, byte_buf_size_ptr, m_last_match_line, &cTokenUncaughtStringTypes};
                            }
                            unvisited_states.pop();
                            visited_states.insert(current_state);
                            for (uint32_t byte = 0; byte < cSizeOfByte; byte++) {
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
                    return Token{m_start_pos, m_match_pos, byte_buf_ptr, byte_buf_size_ptr, m_match_line, m_type_ids};
                }
            }
            state = next;
        }
    }

// If reset() is called all Tokens previously created by the lexer are invalid
    void Lexer::reset (ReaderInterface& reader_interface) {
        reader = &reader_interface;
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
        current_buff_size = cStaticByteBuffSize;
        byte_buf_ptr = &static_byte_buf_ptr;
        byte_buf_size_ptr = &cStaticByteBuffSize;

        reader->read(active_byte_buf, current_buff_size / 2, bytes_read);
        if (bytes_read < current_buff_size / 2) {
            finished_reading_file = true;
        }
        m_fail_pos = current_buff_size / 2;
        m_match_pos = 0;
        m_start_pos = 0;
        m_match_line = 0;
        m_last_match_line = 0;
        m_type_ids = nullptr;
    }

    void Lexer::add_delimiters (const std::vector<uint32_t>& delimiters) {
        assert(!delimiters.empty());
        has_delimiters = true;
        for (bool& i: is_delimiter) {
            i = false;
        }
        for (uint32_t delimiter: delimiters) {
            is_delimiter[delimiter] = true;
        }
    }

    void Lexer::add_rule (const uint32_t& id, std::unique_ptr<RegexAST> rule) {
        rules.emplace_back(id, std::move(rule));
    }

    RegexAST* Lexer::get_rule (const uint32_t& name) {
        for (Rule& rule: rules) {
            if (rule.name == name) {
                return rule.regex.get();
            }
        }
        return nullptr;
    }

    void Lexer::generate () {
        RegexNFA nfa;
        for (const Rule& r: rules) {
            r.add_accepting_state(&nfa);
        }
        dfa = nfa.to_dfa();

        RegexDFA::State* state = dfa->root();
        for (uint32_t i = 0; i < cSizeOfByte; i++) {
            if (state->byte_transitions[i] != nullptr) {
                is_first_char[i] = true;
            } else {
                is_first_char[i] = false;
            }
        }
    }

    void Lexer::generate_reverse () {
        RegexNFA nfa;
        for (const Rule& r: rules) {
            r.add_accepting_state(&nfa);
        }
        nfa.reverse();
        dfa = nfa.to_dfa();

        RegexDFA::State* state = dfa->root();
        for (uint32_t i = 0; i < cSizeOfByte; i++) {
            if (state->byte_transitions[i] != nullptr) {
                is_first_char[i] = true;
            } else {
                is_first_char[i] = false;
            }
        }
    }

    void Lexer::Rule::add_accepting_state (RegexNFA* nfa) const {
        RegexNFA::State* s = nfa->new_state();
        s->set_accepting(true);
        s->set_tag(name);
        regex->add_state(nfa, s);
    }
}