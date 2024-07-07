#include <string>

#include <Catch2/single_include/catch2/catch.hpp>
#include <log_surgeon/Lexer.hpp>
#include <log_surgeon/SchemaParser.hpp>

#include "../src/clp/Grep.hpp"

using clp::Grep;
using clp::load_lexer_from_file;
using log_surgeon::DelimiterStringAST;
using log_surgeon::lexers::ByteLexer;
using log_surgeon::ParserAST;
using log_surgeon::SchemaAST;
using log_surgeon::SchemaParser;
using log_surgeon::SchemaVarAST;
using std::string;

TEST_CASE("get_bounds_of_next_potential_var", "[get_bounds_of_next_potential_var]") {
    ByteLexer forward_lexer;
    load_lexer_from_file("../tests/test_schema_files/search_schema.txt", false, forward_lexer);
    ByteLexer reverse_lexer;
    load_lexer_from_file("../tests/test_schema_files/search_schema.txt", true, reverse_lexer);

    string str;
    size_t begin_pos;
    size_t end_pos;
    bool is_var;

    // m_end_pos past the end of the string
    str = "";
    begin_pos = string::npos;
    end_pos = string::npos;
    REQUIRE(Grep::get_bounds_of_next_potential_var(
                    str,
                    begin_pos,
                    end_pos,
                    is_var,
                    forward_lexer,
                    reverse_lexer
            )
            == false);

    // Empty string
    str = "";
    begin_pos = 0;
    end_pos = 0;
    REQUIRE(Grep::get_bounds_of_next_potential_var(
                    str,
                    begin_pos,
                    end_pos,
                    is_var,
                    forward_lexer,
                    reverse_lexer
            )
            == false);

    // No tokens
    str = "=";
    begin_pos = 0;
    end_pos = 0;
    REQUIRE(Grep::get_bounds_of_next_potential_var(
                    str,
                    begin_pos,
                    end_pos,
                    is_var,
                    forward_lexer,
                    reverse_lexer
            )
            == false);

    // No wildcards
    str = " MAC address 95: ad ff 95 24 0d ff =-abc- ";
    begin_pos = 0;
    end_pos = 0;

    REQUIRE(Grep::get_bounds_of_next_potential_var(
                    str,
                    begin_pos,
                    end_pos,
                    is_var,
                    forward_lexer,
                    reverse_lexer
            )
            == true);
    REQUIRE("95" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(
                    str,
                    begin_pos,
                    end_pos,
                    is_var,
                    forward_lexer,
                    reverse_lexer
            )
            == true);
    REQUIRE("ad" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(
                    str,
                    begin_pos,
                    end_pos,
                    is_var,
                    forward_lexer,
                    reverse_lexer
            )
            == true);
    REQUIRE("ff" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(
                    str,
                    begin_pos,
                    end_pos,
                    is_var,
                    forward_lexer,
                    reverse_lexer
            )
            == true);
    REQUIRE("95" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(
                    str,
                    begin_pos,
                    end_pos,
                    is_var,
                    forward_lexer,
                    reverse_lexer
            )
            == true);
    REQUIRE("24" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(
                    str,
                    begin_pos,
                    end_pos,
                    is_var,
                    forward_lexer,
                    reverse_lexer
            )
            == true);
    REQUIRE("0d" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(
                    str,
                    begin_pos,
                    end_pos,
                    is_var,
                    forward_lexer,
                    reverse_lexer
            )
            == true);
    REQUIRE("ff" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("-abc-" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(
                    str,
                    begin_pos,
                    end_pos,
                    is_var,
                    forward_lexer,
                    reverse_lexer
            )
            == false);
    REQUIRE(str.length() == begin_pos);

    // With wildcards
    str = "~=1\\*x\\?!abc*123;1.2%x:+394/-=-*abc-";
    begin_pos = 0;
    end_pos = 0;

    REQUIRE(Grep::get_bounds_of_next_potential_var(
                    str,
                    begin_pos,
                    end_pos,
                    is_var,
                    forward_lexer,
                    reverse_lexer
            )
            == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "1\\*x");
    REQUIRE(is_var == true);
    // REQUIRE(is_var == true);

    REQUIRE(Grep::get_bounds_of_next_potential_var(
                    str,
                    begin_pos,
                    end_pos,
                    is_var,
                    forward_lexer,
                    reverse_lexer
            )
            == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "abc*123");
    REQUIRE(is_var == false);
    // REQUIRE(is_var == true);

    REQUIRE(Grep::get_bounds_of_next_potential_var(
                    str,
                    begin_pos,
                    end_pos,
                    is_var,
                    forward_lexer,
                    reverse_lexer
            )
            == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "1.2");
    REQUIRE(is_var == true);

    REQUIRE(Grep::get_bounds_of_next_potential_var(
                    str,
                    begin_pos,
                    end_pos,
                    is_var,
                    forward_lexer,
                    reverse_lexer
            )
            == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "+394/-");
    REQUIRE(is_var == true);

    REQUIRE(Grep::get_bounds_of_next_potential_var(
                    str,
                    begin_pos,
                    end_pos,
                    is_var,
                    forward_lexer,
                    reverse_lexer
            )
            == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "-*abc-");
    REQUIRE(is_var == false);

    REQUIRE(Grep::get_bounds_of_next_potential_var(
                    str,
                    begin_pos,
                    end_pos,
                    is_var,
                    forward_lexer,
                    reverse_lexer
            )
            == false);
}
