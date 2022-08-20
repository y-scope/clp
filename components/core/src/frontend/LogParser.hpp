#ifndef LOGPARSER_HPP
#define LOGPARSER_HPP

#include <cassert>
#include <iostream>

// Boost libraries
#include <boost/filesystem/path.hpp>

#include "LALR1Parser.hpp"
#include "SchemaParser.hpp"

#include "../Stopwatch.hpp"

/*
// Doesn't work anymore, things like std::string(buf + start_pos, buf + Lexer::STATIC_BYTE_BUFF_SIZE) assume size of buffer incorrectly
class TextAST : public ParserAST {
public:
    enum TYPE {
        STATIC,
        INTEGER,
        DOUBLE,
        STRING,
        SIZE,
    };

    TextAST(TYPE type, uint32_t start_pos, uint32_t end_pos, char* buf) : type(type), start_pos(start_pos),
                                                                          end_pos(end_pos), buf(buf) {

    }

    std::string get_delimiter() {
        return std::string(buf + start_pos, buf + start_pos + 1);
    }
    
    std::string get_string() {
        if (get_length() > Lexer::STATIC_BYTE_BUFF_SIZE/2) {
            std::cout << "Error: need to increase byte buff size from " << Lexer::STATIC_BYTE_BUFF_SIZE << " to "
                      << get_length()*2 << std::endl;
            exit(1);
        }
        if (start_pos <= end_pos) {
            return std::string(buf + start_pos, buf + end_pos);
        } else {
            return std::string(buf + start_pos, buf + Lexer::STATIC_BYTE_BUFF_SIZE) + 
                   std::string(buf, buf + end_pos);
        }
    }

    uint32_t get_length() {
        if (start_pos <= end_pos) {
            return end_pos - start_pos;
        } else {
            return Lexer::STATIC_BYTE_BUFF_SIZE - start_pos  + end_pos;
        }
    }

    TYPE type;
    uint32_t start_pos;
    uint32_t end_pos;
    char* buf;
};

class LogMessageAST : public ParserAST {
public:
    LogMessageAST(std::unique_ptr<ParserAST> text_ast) {
        texts.reserve(100);
        texts.push_back(std::move(text_ast));
    }
    void add_text_ast(std::unique_ptr<ParserAST> text_ast) {
        texts.push_back(std::move(text_ast));
    }
    void set_next_message_timestamp(Token* next_message_timestamp_token_ptr) {
        this->next_message_timestamp_token_ptr = next_message_timestamp_token_ptr;
    }

    Token* next_message_timestamp_token_ptr;
    Token* timestamp_token_ptr;
    std::vector<std::unique_ptr<ParserAST>> texts;
};

class LogAST : public ParserAST {
public:
    LogAST(std::unique_ptr<ParserAST> log_message) {
        log_messages.push_back(std::move(log_message));
    }
    LogAST(std::string first_timestamp, std::unique_ptr<ParserAST> log_message) : first_timestamp(std::move(first_timestamp)) {
        log_messages.push_back(std::move(log_message));
    }
    void add_log_message(std::unique_ptr<ParserAST> log_message) {
        log_messages.push_back(std::move(log_message));
    }
    std::string first_timestamp;
    std::vector<std::unique_ptr<ParserAST>> log_messages;
};
*/

class TaggedToken {
public:
    enum TYPE {
        STATIC,
        INTEGER,
        DOUBLE,
        STRING,
        SIZE,
    };
    TYPE type;
    Token token;
    bool is_timestamp = false;
    bool has_delimiter = true;
};

class LogParser : public LALR1Parser {
public:
    static uint32_t newline_timestamp_type_id;
    static uint32_t first_timestamp_type_id;
    static uint32_t integer_type_id;
    static uint32_t newline_type_id;
    static uint32_t double_type_id;
    Token* timestamp_token_ptr;
    uint16_t schema_checksum;
    uint16_t schema_file_size;

    LogParser();
    void init(std::unique_ptr<ParserASTSchemaFile> schema_ast);
    void parse(ReaderInterface* reader);
    void increment_uncompressed_msg_pos(ReaderInterface* reader);
private:
    Token get_next_symbol();
    void add_delimiters(std::unique_ptr<ParserAST> const& delimiters);
    void add_rules(std::unique_ptr<ParserASTSchemaFile> const& schema_ast);
    TaggedToken* active_uncompressed_msg;
    uint32_t uncompressed_msg_size;
    TaggedToken static_uncompressed_msg[Lexer::STATIC_BYTE_BUFF_SIZE];
    uint32_t uncompressed_msg_pos = 0;

};

#endif // LOGPARSER_HPP
