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
    void load_lexer_from_file (const std::string& schema_file_path, bool reverse, Lexer& lexer) {
        FileReader schema_reader;
        schema_reader.try_open(schema_file_path);

        SchemaParser sp;
        unique_ptr<compressor_frontend::SchemaFileAST> schema_ast = sp.generate_schema_ast(schema_reader);
        auto* delimiters_ptr = dynamic_cast<DelimiterStringAST*>(schema_ast->delimiters.get());

        if (!lexer.symbol_id.empty()) {
            throw std::runtime_error("Error: symbol_ids initialized before setting enum symbol_ids");
        }

        lexer.symbol_id[cTokenEnd] = (int) SymbolID::TokenEndID;
        lexer.symbol_id[cTokenUncaughtString] = (int) SymbolID::TokenUncaughtStringID;
        lexer.symbol_id[cTokenInt] = (int) SymbolID::TokenIntId;
        lexer.symbol_id[cTokenDouble] = (int) SymbolID::TokenDoubleId;
        lexer.symbol_id[cTokenFirstTimestamp] = (int) SymbolID::TokenFirstTimestampId;
        lexer.symbol_id[cTokenNewlineTimestamp] = (int) SymbolID::TokenNewlineTimestampId;
        lexer.symbol_id[cTokenNewline] = (int) SymbolID::TokenNewlineId;

        lexer.id_symbol[(int) SymbolID::TokenEndID] = cTokenEnd;
        lexer.id_symbol[(int) SymbolID::TokenUncaughtStringID] = cTokenUncaughtString;
        lexer.id_symbol[(int) SymbolID::TokenIntId] = cTokenInt;
        lexer.id_symbol[(int) SymbolID::TokenDoubleId] = cTokenDouble;
        lexer.id_symbol[(int) SymbolID::TokenFirstTimestampId] = cTokenFirstTimestamp;
        lexer.id_symbol[(int) SymbolID::TokenNewlineTimestampId] = cTokenNewlineTimestamp;
        lexer.id_symbol[(int) SymbolID::TokenNewlineId] = cTokenNewline;


        if (delimiters_ptr != nullptr) {
            lexer.add_delimiters(delimiters_ptr->delimiters);
        }
        for (unique_ptr<ParserAST> const& parser_ast: schema_ast->schema_vars) {
            auto* rule = dynamic_cast<SchemaVarAST*>(parser_ast.get());

            if ("timestamp" == rule->name) {
                continue;
            }

            if (lexer.symbol_id.find(rule->name) == lexer.symbol_id.end()) {
                lexer.symbol_id[rule->name] = lexer.symbol_id.size();
                lexer.id_symbol[lexer.symbol_id[rule->name]] = rule->name;
            }

            // transform '.' from any-character into any non-delimiter character
            rule->regex_ptr->remove_delimiters_from_wildcard(delimiters_ptr->delimiters);

            lexer.add_rule(lexer.symbol_id[rule->name], std::move(rule->regex_ptr));
        }
        if (reverse) {
            lexer.generate_reverse();
        } else {
            lexer.generate();
        }

        schema_reader.close();
    }
}
