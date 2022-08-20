#include<boost/filesystem.hpp>

#include <spdlog/spdlog.h>
#include <iostream>

#include "SchemaParser.hpp"
#include "LogParser.hpp"
#include "../streaming_archive/writer/Archive.hpp"
#include "../clp/utils.hpp"

uint32_t LogParser::newline_timestamp_type_id;
uint32_t LogParser::first_timestamp_type_id;
uint32_t LogParser::integer_type_id;
uint32_t LogParser::newline_type_id;
uint32_t LogParser::double_type_id;

LogParser::LogParser() {
    timestamp_token_ptr = nullptr;
    schema_checksum = 0;
    schema_file_size = 0;
    active_uncompressed_msg = nullptr;
    uncompressed_msg_size = 0;
}

void LogParser::init(std::unique_ptr<ParserASTSchemaFile> schema_ast) {
    active_uncompressed_msg = static_uncompressed_msg;
    add_delimiters(schema_ast->delimiters);
    add_rules(schema_ast);
    m_lexer.generate();
    LogParser::newline_timestamp_type_id = m_lexer.symbol_id["newLineTimestamp"];
    LogParser::first_timestamp_type_id = m_lexer.symbol_id["firstTimestamp"];
    LogParser::integer_type_id = m_lexer.symbol_id["int"];
    LogParser::newline_type_id = m_lexer.symbol_id["newLine"];
    LogParser::double_type_id = m_lexer.symbol_id["double"];
}

void LogParser::add_delimiters(std::unique_ptr<ParserAST> const& delimiters) {
    auto delimiters_ptr = dynamic_cast<DelimiterStringAST*>(delimiters.get());
    if (delimiters_ptr != nullptr) {
        m_lexer.add_delimiters(delimiters_ptr->delimiters);
    }
}

void LogParser::add_rules(std::unique_ptr<ParserASTSchemaFile> const& schema_ast) {
    // Currently, required to have delimiters (if schema_ast->delimiters != nullptr it is already enforced that at least 1 delimiter is specified)
    if(schema_ast->delimiters == nullptr) {
        SPDLOG_ERROR("When using --schema-path, \"delimiters:\" line must be used.");
        throw std::runtime_error("delimiters: line missing");
    }
    std::vector<uint32_t>& delimiters = dynamic_cast<DelimiterStringAST *>(schema_ast->delimiters.get())->delimiters;
    add_token("newLine", '\n');
    for (std::unique_ptr<ParserAST> const& parser_ast : schema_ast->schema_vars) {
        auto rule = dynamic_cast<ParserASTSchemaVar*>(parser_ast.get());
        
        // transform '.' from any-character into any non-delimiter character
        rule->regex_ptr->remove_delimiters_from_wildcard(delimiters);
        
        if (rule->name == "timestamp") {
            std::unique_ptr<RegexAST> first_timestamp_regex_ast(rule->regex_ptr->clone());
            add_rule("firstTimestamp", std::move(first_timestamp_regex_ast));
            std::unique_ptr<RegexAST> newline_timestamp_regex_ast(rule->regex_ptr->clone());
            std::unique_ptr<RegexASTLiteral> r2 = std::make_unique<RegexASTLiteral>('\n');
            add_rule("newLineTimestamp", std::make_unique<RegexASTCat>(std::move(r2), std::move(newline_timestamp_regex_ast)));
            // prevent timestamps from going into the dictionary
            continue;
        } 
        // currently, error out if non-timestamp pattern contains a delimiter
        // check if regex contains a delimiter
        bool is_possible_input[RegexASTGroup::UNICODE_MAX] = {false};
        rule->regex_ptr->set_possible_inputs_to_true(is_possible_input);
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
            ErrorCode error_code = schema_reader.try_open(schema_ast->file_path);
            if (ErrorCode_Success != error_code) {
                SPDLOG_ERROR(schema_ast->file_path + ":" + std::to_string(rule->line_num + 1) + ": error: '" + rule->name
                             + "' has regex pattern which contains delimiter '" + char(delimiter_name) + "'.");
            } else {
                // more detailed debugging based on looking at the file
                std::string line;
                for (uint32_t i = 0; i <= rule->line_num; i++) {
                    schema_reader.read_to_delimiter('\n', false, false, line);
                }
                int colon_pos = 0;
                for (int i = 0; i < line.size(); i++) {
                    colon_pos++;
                    if(line[i] == ':') {
                        break;
                    }
                }
                std::string indent (32, ' ');
                std::string spaces (colon_pos, ' ');
                std::string arrows (line.size() - colon_pos, '^');
                
                SPDLOG_ERROR(schema_ast->file_path + ":" + std::to_string(rule->line_num + 1) + ": error: '" + rule->name
                             + "' has regex pattern which contains delimiter '" + char(delimiter_name) + "'.\n"
                             + indent + line + "\n" + indent + spaces + arrows);
            }
            throw std::runtime_error("Variable contains delimiter");
        }
        
        // if (schema_ast->delimiters != nullptr && rule->name != "equals") {
        // std::vector<uint32_t>& delimiters = dynamic_cast<DelimiterStringAST *>(schema_ast->delimiters.get())->delimiters;
        std::unique_ptr<RegexASTGroup> delimiter_group = std::make_unique<RegexASTGroup>(RegexASTGroup(delimiters));
        rule->regex_ptr = std::make_unique<RegexASTCat>(std::move(delimiter_group), std::move(rule->regex_ptr));
        // }
        add_rule(rule->name, std::move(rule->regex_ptr));
    }
}


void LogParser::increment_uncompressed_msg_pos(ReaderInterface* reader) {
    uncompressed_msg_pos++;
    if (uncompressed_msg_pos == uncompressed_msg_size) {
        std::string warn = "Very long line detected";
        warn += " changing to dynamic uncompressed_msg and increasing size to ";
        warn += std::to_string(uncompressed_msg_size*2);
        SPDLOG_WARN("{}", warn.c_str());
        if (active_uncompressed_msg == static_uncompressed_msg) {
            active_uncompressed_msg = (TaggedToken*) malloc(uncompressed_msg_size * sizeof(TaggedToken));
            memcpy(active_uncompressed_msg, static_uncompressed_msg, sizeof(static_uncompressed_msg));
        }
        uncompressed_msg_size *= 2;
        active_uncompressed_msg = (TaggedToken*) realloc(active_uncompressed_msg, uncompressed_msg_size * sizeof(TaggedToken));
        if (active_uncompressed_msg == nullptr) {
            SPDLOG_ERROR("failed to allocate uncompressed msg of size {}", uncompressed_msg_size);
            std::string err = "Lexer failed to find a match after checking entire buffer";
            err += " in file " + dynamic_cast<FileReader*>(reader)->get_path();
            clp::close_file_and_append_to_segment(*archive_writer);
            dynamic_cast<FileReader*>(reader)->close();
            throw (err); // error of this type will allow the program to continue running to compress other files
        }
    }
}

/////////////////////
///TODO: reserve the id types for the tokens in log_parser so a switch statement can be used
void LogParser::parse(ReaderInterface* reader) {
    uncompressed_msg_pos = 0;
    if (active_uncompressed_msg != static_uncompressed_msg) {
        free(active_uncompressed_msg);
    }
    uncompressed_msg_size = Lexer::STATIC_BYTE_BUFF_SIZE;
    active_uncompressed_msg = static_uncompressed_msg;
    reset(reader);
    m_parse_stack_states.push(root_itemset_ptr);
    active_uncompressed_msg[0].token = get_next_symbol();
    if (active_uncompressed_msg[0].token.type_ids->at(0) == Lexer::TOKEN_END_ID) {
        // DO NOTHING
    } else if (active_uncompressed_msg[0].token.type_ids->at(0) == LogParser::first_timestamp_type_id) {
        active_uncompressed_msg[0].is_timestamp = true;
        increment_uncompressed_msg_pos(reader);
        while (true) {
            active_uncompressed_msg[uncompressed_msg_pos].token = get_next_symbol();
            if (active_uncompressed_msg[uncompressed_msg_pos].token.type_ids->at(0) == LogParser::newline_timestamp_type_id) {
                increment_uncompressed_msg_pos(reader);
                active_uncompressed_msg[uncompressed_msg_pos] = active_uncompressed_msg[uncompressed_msg_pos - 1];
                if (active_uncompressed_msg[uncompressed_msg_pos].token.start_pos == *active_uncompressed_msg[uncompressed_msg_pos].token.buffer_size_ptr - 1) {
                    active_uncompressed_msg[uncompressed_msg_pos].token.start_pos = 0;
                } else {
                    active_uncompressed_msg[uncompressed_msg_pos].token.start_pos++;
                }
                active_uncompressed_msg[uncompressed_msg_pos - 1].token.end_pos =
                        active_uncompressed_msg[uncompressed_msg_pos - 1].token.start_pos + 1;
                active_uncompressed_msg[uncompressed_msg_pos - 1].type = TaggedToken::TYPE::STATIC;
                m_lexer.m_reduce_pos = active_uncompressed_msg[uncompressed_msg_pos].token.start_pos - 1;
                archive_writer->write_msg_using_schema(active_uncompressed_msg, uncompressed_msg_pos,
                                                       m_lexer.has_delimiters);
                active_uncompressed_msg[0].token = active_uncompressed_msg[uncompressed_msg_pos].token;
                uncompressed_msg_pos = 0;
                m_lexer.soft_reset();
            } else if (active_uncompressed_msg[uncompressed_msg_pos].token.type_ids->at(0) == LogParser::newline_type_id) {
                active_uncompressed_msg[uncompressed_msg_pos].type = TaggedToken::TYPE::STATIC;
            } else if (active_uncompressed_msg[uncompressed_msg_pos].token.type_ids->at(0) == LogParser::integer_type_id) {
                active_uncompressed_msg[uncompressed_msg_pos].type = TaggedToken::TYPE::INTEGER;
            } else if (active_uncompressed_msg[uncompressed_msg_pos].token.type_ids->at(0) == LogParser::double_type_id) {
                active_uncompressed_msg[uncompressed_msg_pos].type = TaggedToken::TYPE::DOUBLE;
            } else if (active_uncompressed_msg[uncompressed_msg_pos].token.type_ids->at(0) == Lexer::TOKEN_END_ID) {
                archive_writer->write_msg_using_schema(active_uncompressed_msg, uncompressed_msg_pos,
                                                       m_lexer.has_delimiters);
                break;
            } else if (active_uncompressed_msg[uncompressed_msg_pos].token.type_ids->at(0) == Lexer::TOKEN_UNCAUGHT_STRING_ID) {
                active_uncompressed_msg[uncompressed_msg_pos].type = TaggedToken::TYPE::STATIC;
            } else {
                active_uncompressed_msg[uncompressed_msg_pos].type = TaggedToken::TYPE::STRING;
            }
            increment_uncompressed_msg_pos(reader);
        }
    } else { 
        archive_writer->change_ts_pattern(nullptr);
        active_uncompressed_msg[0].is_timestamp = false;
        active_uncompressed_msg[1].token = active_uncompressed_msg[0].token;
        uncompressed_msg_pos = 2;
        while (true) {
            active_uncompressed_msg[uncompressed_msg_pos].token = get_next_symbol();
            if (active_uncompressed_msg[uncompressed_msg_pos].token.type_ids->at(0) == LogParser::newline_type_id) {
                active_uncompressed_msg[uncompressed_msg_pos].type = TaggedToken::TYPE::STATIC;
            } else if (active_uncompressed_msg[uncompressed_msg_pos].token.type_ids->at(0) == LogParser::integer_type_id) {
                active_uncompressed_msg[uncompressed_msg_pos].type = TaggedToken::TYPE::INTEGER;
            } else if (active_uncompressed_msg[uncompressed_msg_pos].token.type_ids->at(0) == LogParser::double_type_id) {
                active_uncompressed_msg[uncompressed_msg_pos].type = TaggedToken::TYPE::DOUBLE;
            } else if (active_uncompressed_msg[uncompressed_msg_pos].token.type_ids->at(0) == Lexer::TOKEN_UNCAUGHT_STRING_ID) {
                active_uncompressed_msg[uncompressed_msg_pos].type = TaggedToken::TYPE::STATIC;
            } else {
                active_uncompressed_msg[uncompressed_msg_pos].type = TaggedToken::TYPE::STRING;
            }
            
            if (active_uncompressed_msg[uncompressed_msg_pos].token.type_ids->at(0) == Lexer::TOKEN_END_ID) {
                archive_writer->write_msg_using_schema(active_uncompressed_msg, uncompressed_msg_pos,
                                                       m_lexer.has_delimiters);
                break;
            } else if (active_uncompressed_msg[uncompressed_msg_pos].token.type_ids->at(0) == LogParser::newline_type_id) {
                active_uncompressed_msg[uncompressed_msg_pos].type = TaggedToken::TYPE::STATIC;
                m_lexer.m_reduce_pos = active_uncompressed_msg[uncompressed_msg_pos].token.end_pos;
                increment_uncompressed_msg_pos(reader);
                archive_writer->write_msg_using_schema(active_uncompressed_msg, uncompressed_msg_pos,
                                                       m_lexer.has_delimiters);
                uncompressed_msg_pos = 0;
                m_lexer.soft_reset();
            } else if (active_uncompressed_msg[uncompressed_msg_pos].token.get_char(0) == '\n') {
                increment_uncompressed_msg_pos(reader);
                active_uncompressed_msg[uncompressed_msg_pos] = active_uncompressed_msg[uncompressed_msg_pos - 1];
                if (active_uncompressed_msg[uncompressed_msg_pos].token.start_pos == *active_uncompressed_msg[uncompressed_msg_pos].token.buffer_size_ptr - 1) {
                    active_uncompressed_msg[uncompressed_msg_pos].token.start_pos = 0;
                } else {
                    active_uncompressed_msg[uncompressed_msg_pos].token.start_pos++;
                }
                active_uncompressed_msg[uncompressed_msg_pos - 1].token.end_pos =
                        active_uncompressed_msg[uncompressed_msg_pos - 1].token.start_pos + 1;
                active_uncompressed_msg[uncompressed_msg_pos - 1].type = TaggedToken::TYPE::STATIC;
                m_lexer.m_reduce_pos = active_uncompressed_msg[uncompressed_msg_pos].token.start_pos - 1;
                archive_writer->write_msg_using_schema(active_uncompressed_msg, uncompressed_msg_pos,
                                                       m_lexer.has_delimiters);
                active_uncompressed_msg[1] = active_uncompressed_msg[uncompressed_msg_pos];
                active_uncompressed_msg[1].has_delimiter = false;
                uncompressed_msg_pos = 1;
                m_lexer.soft_reset();
            }
            increment_uncompressed_msg_pos(reader);
        }
    }
}

Token LogParser::get_next_symbol() {
        return m_lexer.scan();
}