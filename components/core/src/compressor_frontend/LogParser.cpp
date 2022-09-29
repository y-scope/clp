#include "LogParser.hpp"

// C++ standard libraries
#include <filesystem>
#include <iostream>
#include <spdlog/spdlog.h>

// Project headers
#include "../clp/utils.hpp"
#include "Constants.hpp"
#include "SchemaParser.hpp"

using compressor_frontend::finite_automata::RegexAST;
using compressor_frontend::finite_automata::RegexASTCat;
using compressor_frontend::finite_automata::RegexASTGroup;
using compressor_frontend::finite_automata::RegexASTInteger;
using compressor_frontend::finite_automata::RegexASTLiteral;
using compressor_frontend::finite_automata::RegexASTMultiplication;
using compressor_frontend::finite_automata::RegexASTOr;
using std::make_unique;
using std::runtime_error;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;

namespace compressor_frontend {
    LogParser::LogParser (const string& schema_file_path) {
        m_schema_checksum = 0;
        m_schema_file_size = 0;
        m_active_uncompressed_msg = nullptr;
        m_uncompressed_msg_size = 0;

        std::unique_ptr<compressor_frontend::SchemaFileAST> schema_ast = compressor_frontend::SchemaParser::try_schema_file(schema_file_path);
        add_delimiters(schema_ast->m_delimiters);
        add_rules(schema_ast);
        m_lexer.generate();            
        /// TODO compute checksum can be done inside of generating the schema, and can be part of the lexer (not part of file m_reader), 
        FileReader schema_reader;
        schema_reader.try_open(schema_file_path);
        m_schema_checksum = schema_reader.compute_checksum(m_schema_file_size);
    }

    void LogParser::add_delimiters (const unique_ptr<ParserAST>& delimiters) {
        auto delimiters_ptr = dynamic_cast<DelimiterStringAST*>(delimiters.get());
        if (delimiters_ptr != nullptr) {
            m_lexer.add_delimiters(delimiters_ptr->m_delimiters);
        }
    }

    void LogParser::add_rules (const unique_ptr<SchemaFileAST>& schema_ast) {
        // Currently, required to have delimiters (if schema_ast->delimiters != nullptr it is already enforced that at least 1 delimiter is specified)
        if (schema_ast->m_delimiters == nullptr) {
            throw runtime_error("When using --schema-path, \"delimiters:\" line must be used.");
        }
        vector<uint32_t>& delimiters = dynamic_cast<DelimiterStringAST*>(schema_ast->m_delimiters.get())->m_delimiters;
        add_token("newLine", '\n');
        for (unique_ptr<ParserAST> const& parser_ast: schema_ast->m_schema_vars) {
            auto rule = dynamic_cast<SchemaVarAST*>(parser_ast.get());

            // transform '.' from any-character into any non-delimiter character
            rule->m_regex_ptr->remove_delimiters_from_wildcard(delimiters);

            if (rule->m_name == "timestamp") {
                unique_ptr<RegexAST<RegexNFAByteState>> first_timestamp_regex_ast(rule->m_regex_ptr->clone());
                add_rule("firstTimestamp", std::move(first_timestamp_regex_ast));
                unique_ptr<RegexAST<RegexNFAByteState>> newline_timestamp_regex_ast(rule->m_regex_ptr->clone());
                unique_ptr<RegexASTLiteral<RegexNFAByteState>> r2 = make_unique<RegexASTLiteral<RegexNFAByteState>>('\n');
                add_rule("newLineTimestamp", make_unique<RegexASTCat<RegexNFAByteState>>(std::move(r2), std::move(newline_timestamp_regex_ast)));
                // prevent timestamps from going into the dictionary
                continue;
            }
            // currently, error out if non-timestamp pattern contains a delimiter
            // check if regex contains a delimiter
            bool is_possible_input[cUnicodeMax] = {false};
            rule->m_regex_ptr->set_possible_inputs_to_true(is_possible_input);
            bool contains_delimiter = false;
            uint32_t delimiter_name;
            for (uint32_t delimiter: delimiters) {
                if (is_possible_input[delimiter]) {
                    contains_delimiter = true;
                    delimiter_name = delimiter;
                    break;
                }
            }
            if (contains_delimiter) {
                FileReader schema_reader;
                ErrorCode error_code = schema_reader.try_open(schema_ast->m_file_path);
                if (ErrorCode_Success != error_code) {
                    throw std::runtime_error(schema_ast->m_file_path + ":" + to_string(rule->m_line_num + 1) + ": error: '" + rule->m_name
                                             + "' has regex pattern which contains delimiter '" + char(delimiter_name) + "'.\n");
                } else {
                    // more detailed debugging based on looking at the file
                    string line;
                    for (uint32_t i = 0; i <= rule->m_line_num; i++) {
                        schema_reader.read_to_delimiter('\n', false, false, line);
                    }
                    int colon_pos = 0;
                    for (char i : line) {
                        colon_pos++;
                        if (i == ':') {
                            break;
                        }
                    }
                    string indent(10, ' ');
                    string spaces(colon_pos, ' ');
                    string arrows(line.size() - colon_pos, '^');

                    throw std::runtime_error(schema_ast->m_file_path + ":" + to_string(rule->m_line_num + 1) + ": error: '" + rule->m_name
                                             + "' has regex pattern which contains delimiter '" + char(delimiter_name) + "'.\n"
                                             + indent + line + "\n" + indent + spaces + arrows + "\n");
                }
            }
            unique_ptr<RegexASTGroup<RegexNFAByteState>> delimiter_group =
                    make_unique<RegexASTGroup<RegexNFAByteState>>(RegexASTGroup<RegexNFAByteState>(delimiters));
            rule->m_regex_ptr = make_unique<RegexASTCat<RegexNFAByteState>>(std::move(delimiter_group), std::move(rule->m_regex_ptr));
            add_rule(rule->m_name, std::move(rule->m_regex_ptr));
        }
    }


    void LogParser::increment_uncompressed_msg_pos (ReaderInterface& reader) {
        m_uncompressed_msg_pos++;
        if (m_uncompressed_msg_pos == m_uncompressed_msg_size) {
            string warn = "Very long line detected";
            warn += " changing to dynamic uncompressed_msg and increasing size to ";
            warn += to_string(m_uncompressed_msg_size * 2);
            SPDLOG_WARN("warn");
            if (m_active_uncompressed_msg == m_static_uncompressed_msg) {
                m_active_uncompressed_msg = (Token*) malloc(m_uncompressed_msg_size * sizeof(Token));
                memcpy(m_active_uncompressed_msg, m_static_uncompressed_msg, sizeof(m_static_uncompressed_msg));
            }
            m_uncompressed_msg_size *= 2;
            m_active_uncompressed_msg = (Token*) realloc(m_active_uncompressed_msg, m_uncompressed_msg_size * sizeof(Token));
            if (m_active_uncompressed_msg == nullptr) {
                SPDLOG_ERROR("failed to allocate uncompressed msg of size {}", m_uncompressed_msg_size);
                string err = "Lexer failed to find a match after checking entire buffer";
                err += " in file " + dynamic_cast<FileReader&>(reader).get_path();
                clp::close_file_and_append_to_segment(*m_archive_writer_ptr);
                dynamic_cast<FileReader&>(reader).close();
                throw (err); // error of this type will allow the program to continue running to compress other files
            }
        }
    }

    void LogParser::parse (ReaderInterface& reader) {
        m_uncompressed_msg_pos = 0;
        if (m_active_uncompressed_msg != m_static_uncompressed_msg) {
            free(m_active_uncompressed_msg);
        }
        m_uncompressed_msg_size = cStaticByteBuffSize;
        m_active_uncompressed_msg = m_static_uncompressed_msg;
        reset(reader);
        m_parse_stack_states.push(root_itemset_ptr);
        m_active_uncompressed_msg[0] = get_next_symbol();
        bool has_timestamp = false;
        if (m_active_uncompressed_msg[0].m_type_ids->at(0) == (int) SymbolID::TokenEndID) {
            return;
        }
        if (m_active_uncompressed_msg[0].m_type_ids->at(0) == (int) SymbolID::TokenFirstTimestampId) {
            has_timestamp = true;
            increment_uncompressed_msg_pos(reader);
        } else {
            has_timestamp = false;
            m_archive_writer_ptr->change_ts_pattern(nullptr);
            m_active_uncompressed_msg[1] = m_active_uncompressed_msg[0];
            m_uncompressed_msg_pos = 2;
        }
        while (true) {
            m_active_uncompressed_msg[m_uncompressed_msg_pos] = get_next_symbol();
            int token_type = m_active_uncompressed_msg[m_uncompressed_msg_pos].m_type_ids->at(0);
            if (token_type == (int) SymbolID::TokenEndID) {
                m_archive_writer_ptr->write_msg_using_schema(m_active_uncompressed_msg, m_uncompressed_msg_pos,
                                                             m_lexer.get_has_delimiters(), has_timestamp);
                break;
            }
            bool found_start_of_next_message = (has_timestamp && token_type == (int) SymbolID::TokenNewlineTimestampId) ||
                                               (!has_timestamp && m_active_uncompressed_msg[m_uncompressed_msg_pos].get_char(0) == '\n' &&
                                                token_type != (int) SymbolID::TokenNewlineId);
            bool found_end_of_current_message = !has_timestamp && token_type == (int) SymbolID::TokenNewlineId;
            if (found_end_of_current_message) {
                m_lexer.set_reduce_pos(m_active_uncompressed_msg[m_uncompressed_msg_pos].m_end_pos);
                increment_uncompressed_msg_pos(reader);
                m_archive_writer_ptr->write_msg_using_schema(m_active_uncompressed_msg, m_uncompressed_msg_pos,
                                                             m_lexer.get_has_delimiters(), has_timestamp);
                m_uncompressed_msg_pos = 0;
                m_lexer.soft_reset(NonTerminal::m_next_children_start);
            }
            if (found_start_of_next_message) {
                increment_uncompressed_msg_pos(reader);
                m_active_uncompressed_msg[m_uncompressed_msg_pos] = m_active_uncompressed_msg[m_uncompressed_msg_pos - 1];
                if (m_active_uncompressed_msg[m_uncompressed_msg_pos].m_start_pos == *m_active_uncompressed_msg[m_uncompressed_msg_pos].m_buffer_size_ptr - 1) {
                    m_active_uncompressed_msg[m_uncompressed_msg_pos].m_start_pos = 0;
                } else {
                    m_active_uncompressed_msg[m_uncompressed_msg_pos].m_start_pos++;
                }
                m_active_uncompressed_msg[m_uncompressed_msg_pos - 1].m_end_pos =
                        m_active_uncompressed_msg[m_uncompressed_msg_pos - 1].m_start_pos + 1;
                m_active_uncompressed_msg[m_uncompressed_msg_pos - 1].m_type_ids = &Lexer<RegexNFAByteState, RegexDFAByteState>::cTokenUncaughtStringTypes;
                m_lexer.set_reduce_pos(m_active_uncompressed_msg[m_uncompressed_msg_pos].m_start_pos - 1);
                m_archive_writer_ptr->write_msg_using_schema(m_active_uncompressed_msg, m_uncompressed_msg_pos,
                                                             m_lexer.get_has_delimiters(), has_timestamp);
                // switch to timestamped messages if a timestamp is ever found at the start of line (potentially dangerous as it never switches back)
                /// TODO: potentially switch back if a new line is reached and the message is too long (100x static message size)
                if (token_type == (int) SymbolID::TokenNewlineTimestampId) {
                    has_timestamp = true;
                }
                if (has_timestamp) {
                    m_active_uncompressed_msg[0] = m_active_uncompressed_msg[m_uncompressed_msg_pos];
                    m_uncompressed_msg_pos = 0;
                } else {
                    m_active_uncompressed_msg[1] = m_active_uncompressed_msg[m_uncompressed_msg_pos];
                    m_uncompressed_msg_pos = 1;
                }
                m_lexer.soft_reset(NonTerminal::m_next_children_start);
            }
            increment_uncompressed_msg_pos(reader);
        }
    }

    Token LogParser::get_next_symbol () {
        return m_lexer.scan();
    }
}
