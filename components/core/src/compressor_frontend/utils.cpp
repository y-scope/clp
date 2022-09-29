#include "utils.hpp"

// C++ standard libraries
#include <memory>

// Project headers
#include "../FileReader.hpp"
#include "Constants.hpp"
#include "LALR1Parser.hpp"
#include "SchemaParser.hpp"

using std::unique_ptr;

namespace compressor_frontend {
    void load_lexer_from_file (const std::string& schema_file_path, bool reverse, lexers::ByteLexer& lexer) {
        FileReader schema_reader;
        schema_reader.try_open(schema_file_path);

        SchemaParser sp;
        unique_ptr<compressor_frontend::SchemaFileAST> schema_ast = sp.generate_schema_ast(schema_reader);
        auto* delimiters_ptr = dynamic_cast<DelimiterStringAST*>(schema_ast->m_delimiters.get());

        if (!lexer.m_symbol_id.empty()) {
            throw std::runtime_error("Error: symbol_ids initialized before setting enum symbol_ids");
        }

        /// TODO: this is a copy of other code
        lexer.m_symbol_id[cTokenEnd] = (int) SymbolID::TokenEndID;
        lexer.m_symbol_id[cTokenUncaughtString] = (int) SymbolID::TokenUncaughtStringID;
        lexer.m_symbol_id[cTokenInt] = (int) SymbolID::TokenIntId;
        lexer.m_symbol_id[cTokenDouble] = (int) SymbolID::TokenDoubleId;
        lexer.m_symbol_id[cTokenFirstTimestamp] = (int) SymbolID::TokenFirstTimestampId;
        lexer.m_symbol_id[cTokenNewlineTimestamp] = (int) SymbolID::TokenNewlineTimestampId;
        lexer.m_symbol_id[cTokenNewline] = (int) SymbolID::TokenNewlineId;

        lexer.m_id_symbol[(int) SymbolID::TokenEndID] = cTokenEnd;
        lexer.m_id_symbol[(int) SymbolID::TokenUncaughtStringID] = cTokenUncaughtString;
        lexer.m_id_symbol[(int) SymbolID::TokenIntId] = cTokenInt;
        lexer.m_id_symbol[(int) SymbolID::TokenDoubleId] = cTokenDouble;
        lexer.m_id_symbol[(int) SymbolID::TokenFirstTimestampId] = cTokenFirstTimestamp;
        lexer.m_id_symbol[(int) SymbolID::TokenNewlineTimestampId] = cTokenNewlineTimestamp;
        lexer.m_id_symbol[(int) SymbolID::TokenNewlineId] = cTokenNewline;

        /// TODO: figure out why this needs to be specially added
        lexer.add_rule(lexer.m_symbol_id["newLine"],
                       std::move(make_unique<RegexASTLiteral<finite_automata::RegexNFAByteState>>(RegexASTLiteral<finite_automata::RegexNFAByteState>('\n'))));

        if (delimiters_ptr != nullptr) {
            lexer.add_delimiters(delimiters_ptr->m_delimiters);
        }
        for (unique_ptr<ParserAST> const& parser_ast: schema_ast->m_schema_vars) {
            auto* rule = dynamic_cast<SchemaVarAST*>(parser_ast.get());

            if ("timestamp" == rule->m_name) {
                continue;
            }

            if (lexer.m_symbol_id.find(rule->m_name) == lexer.m_symbol_id.end()) {
                lexer.m_symbol_id[rule->m_name] = lexer.m_symbol_id.size();
                lexer.m_id_symbol[lexer.m_symbol_id[rule->m_name]] = rule->m_name;
            }

            // transform '.' from any-character into any non-delimiter character
            rule->m_regex_ptr->remove_delimiters_from_wildcard(delimiters_ptr->m_delimiters);

            /// TODO: this error function is a copy
            // currently, error out if non-timestamp pattern contains a delimiter
            // check if regex contains a delimiter
            bool is_possible_input[cUnicodeMax] = {false};
            rule->m_regex_ptr->set_possible_inputs_to_true(is_possible_input);
            bool contains_delimiter = false;
            uint32_t delimiter_name;
            for (uint32_t delimiter: delimiters_ptr->m_delimiters) {
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
                    throw std::runtime_error(schema_file_path + ":" + to_string(rule->m_line_num + 1) + ": error: '" + rule->m_name
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

                    throw std::runtime_error(schema_file_path + ":" + to_string(rule->m_line_num + 1) + ": error: '" + rule->m_name
                                             + "' has regex pattern which contains delimiter '" + char(delimiter_name) + "'.\n"
                                             + indent + line + "\n" + indent + spaces + arrows + "\n");

                }
            }

            lexer.add_rule(lexer.m_symbol_id[rule->m_name], std::move(rule->m_regex_ptr));
        }
        if (reverse) {
            lexer.generate_reverse();
        } else {
            lexer.generate();
        }

        schema_reader.close();
    }
}
