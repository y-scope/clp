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
    void load_lexer_from_file (const std::string& schema_file_path, bool reverse, Lexer<RegexNFAByteState, RegexDFAByteState>& lexer) {
        FileReader schema_reader;
        schema_reader.try_open(schema_file_path);

        SchemaParser sp;
        unique_ptr<compressor_frontend::SchemaFileAST> schema_ast = sp.generate_schema_ast(schema_reader);
        auto* delimiters_ptr = dynamic_cast<DelimiterStringAST*>(schema_ast->m_delimiters.get());

        if (!lexer.m_symbol_id.empty()) {
            throw std::runtime_error("Error: symbol_ids initialized before setting enum symbol_ids");
        }

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
