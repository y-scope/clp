// C++ standard libraries
#include <memory>

// Project headers
#include "../FileReader.hpp"
#include "LALR1Parser.hpp"
#include "SchemaParser.hpp"

using std::unique_ptr;

namespace frontend {
    void load_lexer_from_file (const std::string& schema_file_path, bool reverse, Lexer& lexer) {
        FileReader schema_reader;
        schema_reader.try_open(schema_file_path);

        SchemaParser sp;
        unique_ptr<ParserASTSchemaFile> schema_ast = sp.generate_schema_ast(&schema_reader);
        auto* delimiters_ptr = dynamic_cast<DelimiterStringAST*>(schema_ast->delimiters.get());

        lexer.symbol_id[Lexer::TOKEN_END] = lexer.symbol_id.size();
        lexer.symbol_id[Lexer::TOKEN_UNCAUGHT_STRING] = lexer.symbol_id.size();
        lexer.id_symbol[Lexer::TOKEN_END_ID] = Lexer::TOKEN_END;
        lexer.id_symbol[Lexer::TOKEN_UNCAUGHT_STRING_ID] = Lexer::TOKEN_UNCAUGHT_STRING;
        if (delimiters_ptr != nullptr) {
            lexer.add_delimiters(delimiters_ptr->delimiters);
        }
        for (unique_ptr<ParserAST> const& parser_ast : schema_ast->schema_vars) {
            auto* rule = dynamic_cast<ParserASTSchemaVar*>(parser_ast.get());

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
