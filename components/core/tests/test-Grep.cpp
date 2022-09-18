// C++ standard libraries
#include <string>

// Catch2
#include "../submodules/Catch2/single_include/catch2/catch.hpp"

// Project headers
#include "../src/compressor_frontend/Lexer.hpp"
#include "../src/compressor_frontend/SchemaParser.hpp"
#include "../src/Grep.hpp"

using compressor_frontend::DelimiterStringAST;
using compressor_frontend::Lexer;
using compressor_frontend::ParserAST;
using compressor_frontend::SchemaFileAST;
using compressor_frontend::SchemaParser;
using compressor_frontend::SchemaVarAST;
using std::string;

TEST_CASE("get_bounds_of_next_potential_var", "[get_bounds_of_next_potential_var]") {
    Lexer forward_lexer;
    {
        FileReader schema_reader;
        ErrorCode error_code = schema_reader.try_open("../tests/test_schema_files/search_schema.txt");
        SchemaParser sp;
        std::unique_ptr<SchemaFileAST> schema_ast = sp.generate_schema_ast(schema_reader);
        auto delimiters_ptr = dynamic_cast<DelimiterStringAST*>(schema_ast->delimiters.get());
        // Create forward lexer
        forward_lexer.symbol_id[compressor_frontend::cTokenEnd] = forward_lexer.symbol_id.size();
        forward_lexer.symbol_id[compressor_frontend::cTokenUncaughtString] = forward_lexer.symbol_id.size();
        forward_lexer.id_symbol[(int)compressor_frontend::SymbolID::TokenEndID] = compressor_frontend::cTokenEnd;
        forward_lexer.id_symbol[(int)compressor_frontend::SymbolID::TokenUncaughtStringID] = compressor_frontend::cTokenUncaughtString;
        if (delimiters_ptr != nullptr) {
            forward_lexer.add_delimiters(delimiters_ptr->delimiters);
        }
        for (std::unique_ptr<ParserAST> const &parser_ast: schema_ast->schema_vars) {
            auto rule = dynamic_cast<SchemaVarAST*>(parser_ast.get());
            if (forward_lexer.symbol_id.find(rule->name) == forward_lexer.symbol_id.end()) {
                forward_lexer.symbol_id[rule->name] = forward_lexer.symbol_id.size();
                forward_lexer.id_symbol[forward_lexer.symbol_id[rule->name]] = rule->name;

            }
            forward_lexer.add_rule(forward_lexer.symbol_id[rule->name], std::move(rule->regex_ptr));
        }
        forward_lexer.generate();
        schema_reader.close();
    }
    // Create reverse lexer
    Lexer reverse_lexer;
    {
        FileReader schema_reader;
        ErrorCode error_code = schema_reader.try_open("../tests/test_schema_files/search_schema.txt");
        SchemaParser sp;
        std::unique_ptr<SchemaFileAST> schema_ast = sp.generate_schema_ast(schema_reader);
        auto delimiters_ptr = dynamic_cast<DelimiterStringAST*>(schema_ast->delimiters.get());
        reverse_lexer.symbol_id[compressor_frontend::cTokenEnd] = reverse_lexer.symbol_id.size();
        reverse_lexer.symbol_id[compressor_frontend::cTokenUncaughtString] = reverse_lexer.symbol_id.size();
        reverse_lexer.id_symbol[(int)compressor_frontend::SymbolID::TokenEndID] = compressor_frontend::cTokenEnd;
        reverse_lexer.id_symbol[(int)compressor_frontend::SymbolID::TokenUncaughtStringID] = compressor_frontend::cTokenUncaughtString;
        if (delimiters_ptr != nullptr) {
            reverse_lexer.add_delimiters(delimiters_ptr->delimiters);
        }
        for (std::unique_ptr<ParserAST> const &parser_ast: schema_ast->schema_vars) {
            auto rule = dynamic_cast<SchemaVarAST*>(parser_ast.get());
            if (reverse_lexer.symbol_id.find(rule->name) == reverse_lexer.symbol_id.end()) {
                reverse_lexer.symbol_id[rule->name] = reverse_lexer.symbol_id.size();
                reverse_lexer.id_symbol[reverse_lexer.symbol_id[rule->name]] = rule->name;
            }
            reverse_lexer.add_rule(reverse_lexer.symbol_id[rule->name], std::move(rule->regex_ptr));
        }
        reverse_lexer.generate_reverse();
        schema_reader.close();
    }

    string str;
    size_t begin_pos;
    size_t end_pos;
    bool is_var;

    // end_pos past the end of the string
    str = "";
    begin_pos = string::npos;
    end_pos = string::npos;
    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var, forward_lexer, reverse_lexer) == false);

    // Empty string
    str = "";
    begin_pos = 0;
    end_pos = 0;
    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var, forward_lexer, reverse_lexer) == false);

    // No tokens
    str = "=";
    begin_pos = 0;
    end_pos = 0;
    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var, forward_lexer, reverse_lexer) == false);

    // No wildcards
    str = " MAC address 95: ad ff 95 24 0d ff =-abc- ";
    begin_pos = 0;
    end_pos = 0;

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var, forward_lexer, reverse_lexer) == true);
    REQUIRE("95" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var, forward_lexer, reverse_lexer) == true);
    REQUIRE("ad" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var, forward_lexer, reverse_lexer) == true);
    REQUIRE("ff" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var, forward_lexer, reverse_lexer) == true);
    REQUIRE("95" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var, forward_lexer, reverse_lexer) == true);
    REQUIRE("24" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var, forward_lexer, reverse_lexer) == true);
    REQUIRE("0d" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var, forward_lexer, reverse_lexer) == true);
    REQUIRE("ff" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("-abc-" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var, forward_lexer, reverse_lexer) == false);
    REQUIRE(str.length() == begin_pos);

    // With wildcards
    str = "~=\\*x\\?!abc*123;1.2%x:+394/-=-*abc-";
    begin_pos = 0;
    end_pos = 0;

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var, forward_lexer, reverse_lexer) == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "=\\*x");
    REQUIRE(is_var == true);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var, forward_lexer, reverse_lexer) == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "abc*123");
    REQUIRE(is_var == false);
    //REQUIRE(is_var == true);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var, forward_lexer, reverse_lexer) == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "1.2");
    REQUIRE(is_var == true);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var, forward_lexer, reverse_lexer) == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "+394/-");
    REQUIRE(is_var == true);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var, forward_lexer, reverse_lexer) == false);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "-*abc-");
    REQUIRE(is_var == false);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var, forward_lexer, reverse_lexer) == false);
}
