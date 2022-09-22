#include "Lexer.hpp"

// C++ standard libraries
#include <cassert>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

// Project headers
#include "../streaming_archive/writer/Archive.hpp"
#include "Constants.hpp"
#include "finite_automata/RegexAST.hpp"
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
    uint32_t Lexer::m_current_buff_size;

    void Lexer::soft_reset () {
        if (NonTerminal::m_next_children_start > cSizeOfAllChildren / 2) {
            NonTerminal::m_next_children_start = 0;
        }
        if (m_finished_reading_file) {
            return;
        }
        if (m_reduce_pos == -1) {
            m_reduce_pos += m_current_buff_size;
        }
        if ((!m_last_read_first_half_of_buf && m_reduce_pos > m_current_buff_size / 2) ||
            (m_last_read_first_half_of_buf && m_reduce_pos < m_current_buff_size / 2 && m_reduce_pos > 0)) {
            uint32_t offset = 0;
            if (m_last_read_first_half_of_buf) {
                offset = m_current_buff_size / 2;
            }
            m_reader->read(m_active_byte_buf + offset, m_current_buff_size / 2, m_bytes_read);

            if (m_bytes_read < m_current_buff_size / 2) {
                m_finished_reading_file = true;
            }
            m_last_read_first_half_of_buf = !m_last_read_first_half_of_buf;
            m_bytes_read += offset;
            if (m_reduce_pos >= m_current_buff_size / 2) {
                m_fail_pos = m_current_buff_size / 2;
            } else {
                m_fail_pos = 0;
            }
        }
    }

    unsigned char Lexer::get_next_character () {
        if (m_finished_reading_file && m_byte_buf_pos == m_bytes_read) {
            m_at_end_of_file = true;
            return utf8::cCharEOF;
        }
        unsigned char character = m_active_byte_buf[m_byte_buf_pos];
        m_byte_buf_pos++;
        if (m_byte_buf_pos == m_current_buff_size) {
            m_byte_buf_pos = 0;
        }
        return character;
    }

    Token Lexer::scan () {
        if (m_match) {
            m_match = false;
            m_last_match_pos = m_match_pos;
            m_last_match_line = m_match_line;
            return Token{m_start_pos, m_match_pos, m_byte_buf_ptr, m_byte_buf_size_ptr, m_match_line, m_type_ids};
        }
        m_start_pos = m_byte_buf_pos;
        m_match_pos = m_byte_buf_pos;
        m_match_line = m_line;
        m_type_ids = nullptr;
        RegexDFA::State* state = m_dfa->get_root();
        while (true) {
            if (m_byte_buf_pos == m_fail_pos) {
                string warn = "Long line detected";
                warn += " at line " + to_string(m_line);
                warn += " in file " + dynamic_cast<FileReader*>(m_reader)->get_path();
                warn += " changing to dynamic buffer and increasing buffer size to ";
                warn += to_string(m_current_buff_size * 2);
                SPDLOG_WARN(warn);
                // Found a super long line: for completeness handle this case, but efficiency doesn't matter
                // 1. copy everything from old buffer into new buffer
                if (m_active_byte_buf == m_static_byte_buf) {
                    m_active_byte_buf = (char*) malloc(m_current_buff_size * sizeof(char));
                    if (m_fail_pos == 0) {
                        memcpy(m_active_byte_buf, m_static_byte_buf, sizeof(m_static_byte_buf));
                    } else {
                        //TODO: make a test case for this scenario
                        memcpy(m_active_byte_buf, m_static_byte_buf + sizeof(m_static_byte_buf) / 2, sizeof(m_static_byte_buf) / 2);
                        memcpy(m_active_byte_buf + sizeof(m_static_byte_buf) / 2, m_static_byte_buf, sizeof(m_static_byte_buf) / 2);
                        if (m_match_pos >= m_current_buff_size / 2) {
                            m_match_pos -= m_current_buff_size / 2;
                        } else {
                            m_match_pos += m_current_buff_size / 2;
                        }
                        if (m_start_pos >= m_current_buff_size / 2) {
                            m_start_pos -= m_current_buff_size / 2;
                        } else {
                            m_start_pos += m_current_buff_size / 2;
                        }
                        if (m_last_match_pos >= m_current_buff_size / 2) {
                            m_last_match_pos -= m_current_buff_size / 2;
                        } else {
                            m_last_match_pos += m_current_buff_size / 2;
                        }
                    }
                }
                m_current_buff_size *= 2;
                m_active_byte_buf = (char*) realloc(m_active_byte_buf, m_current_buff_size * sizeof(char));
                m_byte_buf_ptr = &m_active_byte_buf;
                m_byte_buf_size_ptr = &m_current_buff_size;
                if (m_active_byte_buf == nullptr) {
                    SPDLOG_ERROR("failed to allocate byte buffer of size {}", m_current_buff_size);
                    string err = "Lexer failed to find a match after checking entire buffer";
                    err += " at line " + to_string(m_line);
                    err += " in file " + dynamic_cast<FileReader*>(m_reader)->get_path();
                    dynamic_cast<FileReader*>(m_reader)->close();
                    throw (err); // this throw allows for continuation of compressing other files 
                }
                m_reader->read(m_active_byte_buf + m_current_buff_size / 2, m_current_buff_size / 2, m_bytes_read);
                m_bytes_read += m_current_buff_size / 2;
                if (m_bytes_read < m_current_buff_size) {
                    m_finished_reading_file = true;
                }
                m_byte_buf_pos = m_current_buff_size / 2;
                m_fail_pos = 0;
            }
            uint32_t prev_byte_buf_pos = m_byte_buf_pos;
            unsigned char next_char = get_next_character();
            if ((m_is_delimiter[next_char] || m_at_end_of_file || !m_has_delimiters) && state->is_accepting()) {
                m_match = true;
                m_type_ids = &(state->get_tags());
                m_match_pos = prev_byte_buf_pos;
                m_match_line = m_line;
            }
            RegexDFA::State* next = state->next(next_char);
            if (next_char == '\n') {
                m_line++;
                if (m_has_delimiters && !m_match) {
                    next = m_dfa->get_root()->next(next_char);
                    m_match = true;
                    m_type_ids = &(next->get_tags());
                    m_start_pos = prev_byte_buf_pos;
                    m_match_pos = m_byte_buf_pos;
                    m_match_line = m_line;
                }
            }
            if (m_at_end_of_file || next == nullptr) {
                if (m_match) {
                    m_at_end_of_file = false;
                    m_byte_buf_pos = m_match_pos;
                    m_line = m_match_line;
                    if (m_last_match_pos != m_start_pos) {
                        return Token{m_last_match_pos, m_start_pos, m_byte_buf_ptr, m_byte_buf_size_ptr, m_last_match_line, &cTokenUncaughtStringTypes};
                    }
                    m_match = false;
                    m_last_match_pos = m_match_pos;
                    m_last_match_line = m_match_line;
                    return Token{m_start_pos, m_match_pos, m_byte_buf_ptr, m_byte_buf_size_ptr, m_match_line, m_type_ids};
                } else if (m_at_end_of_file && m_start_pos == m_byte_buf_pos) {
                    if (m_last_match_pos != m_start_pos) {
                        m_match_pos = m_byte_buf_pos;
                        m_type_ids = &cTokenEndTypes;
                        m_match = true;
                        return Token{m_last_match_pos, m_start_pos, m_byte_buf_ptr, m_byte_buf_size_ptr, m_last_match_line, &cTokenUncaughtStringTypes};
                    }
                    return Token{m_byte_buf_pos, m_byte_buf_pos, m_byte_buf_ptr, m_byte_buf_size_ptr, m_line, &cTokenEndTypes};
                } else {
                    while (!m_at_end_of_file && !m_is_first_char[next_char]) {
                        prev_byte_buf_pos = m_byte_buf_pos;
                        next_char = get_next_character();
                    }
                    m_byte_buf_pos = prev_byte_buf_pos;
                    m_start_pos = prev_byte_buf_pos;
                    state = m_dfa->get_root();
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
            return Token{m_start_pos, m_match_pos, m_byte_buf_ptr, m_byte_buf_size_ptr, m_match_line, m_type_ids};
        }
        m_start_pos = m_byte_buf_pos;
        m_match_pos = m_byte_buf_pos;
        m_match_line = m_line;
        m_type_ids = nullptr;
        RegexDFA::State* state = m_dfa->get_root();
        while (true) {
            if (m_byte_buf_pos == m_fail_pos) {
                string warn = "Long line detected";
                warn += " at line " + to_string(m_line);
                warn += " in file " + dynamic_cast<FileReader*>(m_reader)->get_path();
                warn += " changing to dynamic buffer and increasing buffer size to ";
                warn += to_string(m_current_buff_size * 2);
                SPDLOG_WARN(warn);
                // Found a super long line: for completeness handle this case, but efficiency doesn't matter
                // 1. copy everything from old buffer into new buffer
                if (m_active_byte_buf == m_static_byte_buf) {
                    m_active_byte_buf = (char*) malloc(m_current_buff_size * sizeof(char));
                    if (m_fail_pos == 0) {
                        memcpy(m_active_byte_buf, m_static_byte_buf, sizeof(m_static_byte_buf));
                    } else {
                        // TODO: make a test case for this scenario
                        memcpy(m_active_byte_buf, m_static_byte_buf + sizeof(m_static_byte_buf) / 2, sizeof(m_static_byte_buf) / 2);
                        memcpy(m_active_byte_buf + sizeof(m_static_byte_buf) / 2, m_static_byte_buf, sizeof(m_static_byte_buf) / 2);
                        if (m_match_pos >= m_current_buff_size / 2) {
                            m_match_pos -= m_current_buff_size / 2;
                        } else {
                            m_match_pos += m_current_buff_size / 2;
                        }
                        if (m_start_pos >= m_current_buff_size / 2) {
                            m_start_pos -= m_current_buff_size / 2;
                        } else {
                            m_start_pos += m_current_buff_size / 2;
                        }
                        if (m_last_match_pos >= m_current_buff_size / 2) {
                            m_last_match_pos -= m_current_buff_size / 2;
                        } else {
                            m_last_match_pos += m_current_buff_size / 2;
                        }
                    }
                }
                m_current_buff_size *= 2;
                m_active_byte_buf = (char*) realloc(m_active_byte_buf, m_current_buff_size * sizeof(char));
                m_byte_buf_ptr = &m_active_byte_buf;
                m_byte_buf_size_ptr = &m_current_buff_size;
                if (m_active_byte_buf == nullptr) {
                    SPDLOG_ERROR("failed to allocate byte buffer of size {}", m_current_buff_size);
                    string err = "Lexer failed to find a match after checking entire buffer";
                    err += " at line " + to_string(m_line);
                    err += " in file " + dynamic_cast<FileReader*>(m_reader)->get_path();
                    dynamic_cast<FileReader*>(m_reader)->close();
                    throw (err); // this throw allows for continuation of compressing other files 
                }
                m_reader->read(m_active_byte_buf + m_current_buff_size / 2, m_current_buff_size / 2, m_bytes_read);
                m_bytes_read += m_current_buff_size / 2;
                if (m_bytes_read < m_current_buff_size) {
                    m_finished_reading_file = true;
                }
                m_byte_buf_pos = m_current_buff_size / 2;
                m_fail_pos = 0;
            }
            uint32_t prev_byte_buf_pos = m_byte_buf_pos;
            unsigned char next_char = get_next_character();
            if ((m_is_delimiter[next_char] || m_at_end_of_file || !m_has_delimiters) && state->is_accepting()) {
                m_match = true;
                m_type_ids = &(state->get_tags());
                m_match_pos = prev_byte_buf_pos;
                m_match_line = m_line;
            }
            RegexDFA::State* next = state->next(next_char);
            if (next_char == '\n') {
                m_line++;
                if (m_has_delimiters && !m_match) {
                    next = m_dfa->get_root()->next(next_char);
                    m_match = true;
                    m_type_ids = &(next->get_tags());
                    m_start_pos = prev_byte_buf_pos;
                    m_match_pos = m_byte_buf_pos;
                    m_match_line = m_line;
                }
            }

            // !m_at_end_of_file should be impossible
            // m_match_pos != m_byte_buf_pos --> "te matches from "tes*" (means "tes" isn't a match, so is_var = false)
            // 
            if (m_at_end_of_file || next == nullptr) {
                assert(m_at_end_of_file);

                if (!m_match || (m_match && m_match_pos != m_byte_buf_pos)) {
                    return Token{m_last_match_pos, m_byte_buf_pos, m_byte_buf_ptr, m_byte_buf_size_ptr, m_last_match_line, &cTokenUncaughtStringTypes};
                }
                if (m_match) {
                    // BFS (keep track of m_type_ids)
                    if (wildcard == '?') {
                        for (uint32_t byte = 0; byte < cSizeOfByte; byte++) {
                            RegexDFA::State* next_state = state->next(byte);
                            if (next_state->is_accepting() == false) {
                                return Token{m_last_match_pos, m_byte_buf_pos, m_byte_buf_ptr, m_byte_buf_size_ptr, m_last_match_line, &cTokenUncaughtStringTypes};
                            }
                        }
                    } else if (wildcard == '*') {
                        std::stack<RegexDFA::State*> unvisited_states;
                        std::set<RegexDFA::State*> visited_states;
                        unvisited_states.push(state);
                        while (!unvisited_states.empty()) {
                            RegexDFA::State* current_state = unvisited_states.top();
                            if (current_state == nullptr || current_state->is_accepting() == false) {
                                return Token{m_last_match_pos, m_byte_buf_pos, m_byte_buf_ptr, m_byte_buf_size_ptr, m_last_match_line, &cTokenUncaughtStringTypes};
                            }
                            unvisited_states.pop();
                            visited_states.insert(current_state);
                            for (uint32_t byte = 0; byte < cSizeOfByte; byte++) {
                                if (m_is_delimiter[byte]) {
                                    continue;
                                }
                                RegexDFA::State* next_state = current_state->next(byte);
                                if (visited_states.find(next_state) == visited_states.end()) {
                                    unvisited_states.push(next_state);
                                }
                            }
                        }
                    }
                    m_byte_buf_pos = m_match_pos;
                    m_line = m_match_line;
                    m_match = false;
                    m_last_match_pos = m_match_pos;
                    m_last_match_line = m_match_line;
                    return Token{m_start_pos, m_match_pos, m_byte_buf_ptr, m_byte_buf_size_ptr, m_match_line, m_type_ids};
                }
            }
            state = next;
        }
    }

// If reset() is called all Tokens previously created by the lexer are invalid
    void Lexer::reset (ReaderInterface& reader_interface) {
        m_reader = &reader_interface;
        m_finished_reading_file = false;
        m_at_end_of_file = false;
        m_reduce_pos = 0;
        m_last_match_pos = 0;
        m_match = false;
        m_byte_buf_pos = 0;
        m_line = 0;
        m_bytes_read = 0;
        m_last_read_first_half_of_buf = true;
        if (m_active_byte_buf != nullptr && m_active_byte_buf != m_static_byte_buf) {
            free(m_active_byte_buf);
        }
        m_static_byte_buf_ptr = m_static_byte_buf;
        m_active_byte_buf = m_static_byte_buf;
        m_current_buff_size = cStaticByteBuffSize;
        m_byte_buf_ptr = &m_static_byte_buf_ptr;
        m_byte_buf_size_ptr = &cStaticByteBuffSize;

        m_reader->read(m_active_byte_buf, m_current_buff_size / 2, m_bytes_read);
        if (m_bytes_read < m_current_buff_size / 2) {
            m_finished_reading_file = true;
        }
        m_fail_pos = m_current_buff_size / 2;
        m_match_pos = 0;
        m_start_pos = 0;
        m_match_line = 0;
        m_last_match_line = 0;
        m_type_ids = nullptr;
    }

    void Lexer::add_delimiters (const std::vector<uint32_t>& delimiters) {
        assert(!delimiters.empty());
        m_has_delimiters = true;
        for (bool& i: m_is_delimiter) {
            i = false;
        }
        for (uint32_t delimiter: delimiters) {
            m_is_delimiter[delimiter] = true;
        }
    }

    void Lexer::add_rule (const uint32_t& id, std::unique_ptr<RegexAST> rule) {
        m_rules.emplace_back(id, std::move(rule));
    }

    RegexAST* Lexer::get_rule (const uint32_t& name) {
        for (Rule& rule: m_rules) {
            if (rule.m_name == name) {
                return rule.m_regex.get();
            }
        }
        return nullptr;
    }

    void Lexer::generate () {
        RegexNFA nfa;
        for (const Rule& r: m_rules) {
            r.add_ast(&nfa);
        }
        m_dfa = nfa.to_dfa();

        RegexDFA::State* state = m_dfa->get_root();
        for (uint32_t i = 0; i < cSizeOfByte; i++) {
            if (state->next(i) != nullptr) {
                m_is_first_char[i] = true;
            } else {
                m_is_first_char[i] = false;
            }
        }
    }

    void Lexer::generate_reverse () {
        RegexNFA nfa;
        for (const Rule& r: m_rules) {
            r.add_ast(&nfa);
        }
        nfa.reverse();
        m_dfa = nfa.to_dfa();

        RegexDFA::State* state = m_dfa->get_root();
        for (uint32_t i = 0; i < cSizeOfByte; i++) {
            if (state->next(i) != nullptr) {
                m_is_first_char[i] = true;
            } else {
                m_is_first_char[i] = false;
            }
        }
    }

    void Lexer::Rule::add_ast (RegexNFA* nfa) const {
        RegexNFA::State* s = nfa->new_state();
        s->set_accepting(true);
        s->set_tag(m_name);
        m_regex->add(nfa, s);
    }
}