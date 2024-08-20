#include <string>

#include <Catch2/single_include/catch2/catch.hpp>
#include <log_surgeon/Lexer.hpp>
#include <log_surgeon/Schema.hpp>
#include <log_surgeon/SchemaParser.hpp>

#include "../src/clp/Grep.hpp"
#include "../src/clp/QueryInterpretation.hpp"
#include "log_surgeon/LogParser.hpp"

using clp::Grep;
using clp::load_lexer_from_file;
using clp::QueryInterpretation;
using log_surgeon::DelimiterStringAST;
using log_surgeon::lexers::ByteLexer;
using log_surgeon::ParserAST;
using log_surgeon::SchemaAST;
using log_surgeon::SchemaParser;
using log_surgeon::SchemaVarAST;
using std::set;
using std::string;
using std::vector;

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
    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == false);

    // Empty string
    str = "";
    begin_pos = 0;
    end_pos = 0;
    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == false);

    // No tokens
    str = "=";
    begin_pos = 0;
    end_pos = 0;
    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == false);

    // No wildcards
    str = " MAC address 95: ad ff 95 24 0d ff =-abc- ";
    begin_pos = 0;
    end_pos = 0;

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("95" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("ad" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("ff" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("95" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("24" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("0d" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("ff" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("-abc-" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == false);
    REQUIRE(str.length() == begin_pos);

    // With wildcards
    str = "~=1\\*x\\?!abc*123;1.2%x:+394/-=-*abc-";
    begin_pos = 0;
    end_pos = 0;

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "1");
    REQUIRE(is_var == true);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "abc*123");
    REQUIRE(is_var == true);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "1.2");
    REQUIRE(is_var == true);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "+394");
    REQUIRE(is_var == true);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "-*abc-");
    REQUIRE(is_var == false);

    REQUIRE(Grep::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == false);
}

// 0:"$end", 1:"$UncaughtString", 2:"int", 3:"float", 4:hex, 5:firstTimestamp, 6:newLineTimestamp,
// 7:timestamp, 8:hex, 9:hasNumber, 10:uniqueVariable, 11:test
TEST_CASE("get_substring_variable_types", "[schema_search]") {
    ByteLexer lexer;
    clp::load_lexer_from_file("../tests/test_schema_files/search_schema.txt", false, lexer);

    SECTION("* 10000 reply: *") {
        string query = "* 10000 reply: *";
        auto [is_greedy_wildcard, is_non_greedy_wildcard, is_escape]
                = Grep::get_wildcard_and_escape_locations(query);
        for (uint32_t end_idx = 1; end_idx <= query.size(); end_idx++) {
            for (uint32_t begin_idx = 0; begin_idx < end_idx; begin_idx++) {
                auto [variable_types, contains_wildcard] = Grep::get_substring_variable_types(
                        query.substr(begin_idx, end_idx - begin_idx),
                        begin_idx,
                        is_greedy_wildcard,
                        is_non_greedy_wildcard,
                        is_escape,
                        lexer
                );
                std::set<uint32_t> expected_variable_types;
                // "*"
                if ((0 == begin_idx && 1 == end_idx)
                    || (query.size() - 1 == begin_idx && query.size() == end_idx))
                {
                    expected_variable_types
                            = {lexer.m_symbol_id["timestamp"],
                               lexer.m_symbol_id["int"],
                               lexer.m_symbol_id["float"],
                               lexer.m_symbol_id["hex"],
                               lexer.m_symbol_id["hasNumber"],
                               lexer.m_symbol_id["uniqueVariable"],
                               lexer.m_symbol_id["test"]};
                }
                // substrings of "10000"
                if (2 <= begin_idx && 7 >= end_idx) {
                    expected_variable_types
                            = {lexer.m_symbol_id["int"], lexer.m_symbol_id["hasNumber"]};
                }
                //"e"
                if (9 == begin_idx && 10 == end_idx) {
                    expected_variable_types = {lexer.m_symbol_id["hex"]};
                }
                bool expected_contains_wildcard = false;
                if (0 == begin_idx || query.size() == end_idx) {
                    expected_contains_wildcard = true;
                }
                CAPTURE(query.substr(begin_idx, end_idx - begin_idx));
                CAPTURE(begin_idx);
                CAPTURE(end_idx);
                REQUIRE(variable_types == expected_variable_types);
                REQUIRE(contains_wildcard == expected_contains_wildcard);
            }
        }
    }
}

TEST_CASE("get_possible_substr_types", "[schema_search]") {
    ByteLexer lexer;
    clp::load_lexer_from_file("../tests/test_schema_files/search_schema.txt", false, lexer);

    SECTION("* 10000 reply: *") {
        string query = "* 10000 reply: *";
        auto [is_greedy_wildcard, is_non_greedy_wildcard, is_escape]
                = Grep::get_wildcard_and_escape_locations(query);
        for (uint32_t end_idx = 1; end_idx <= query.size(); end_idx++) {
            for (uint32_t begin_idx = 0; begin_idx < end_idx; begin_idx++) {
                auto query_logtypes = Grep::get_possible_substr_types(
                        query,
                        begin_idx,
                        end_idx,
                        is_greedy_wildcard,
                        is_non_greedy_wildcard,
                        is_escape,
                        lexer
                );
                vector<QueryInterpretation> expected_result(0);
                if (2 == begin_idx && 7 == end_idx) {
                    expected_result.emplace_back();
                    expected_result[0].append_variable_token(
                            static_cast<int>(lexer.m_symbol_id["int"]),
                            "10000",
                            false,
                            false
                    );
                } else if ((0 != begin_idx && query.size() != end_idx)
                           || (end_idx - begin_idx == 1))
                {
                    expected_result.emplace_back();
                    for (uint32_t idx = begin_idx; idx < end_idx; idx++) {
                        expected_result[0].append_static_token(query.substr(idx, 1));
                    }
                }
                CAPTURE(begin_idx);
                CAPTURE(end_idx);
                REQUIRE(query_logtypes == expected_result);
            }
        }
    }
}

TEST_CASE(
        "generate_query_substring_interpretations",
        "[generate_query_substring_interpretations][schema_search]"
) {
    ByteLexer lexer;
    clp::load_lexer_from_file("../tests/test_schema_files/search_schema.txt", false, lexer);

    SECTION("Static text") {
        string query = "* z *";
        auto const query_logtypes = Grep::generate_query_substring_interpretations(query, lexer);
        set<QueryInterpretation> expected_result;
        // "* z *"
        QueryInterpretation query_interpretation;
        query_interpretation.append_static_token("* z *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        REQUIRE(query_logtypes == expected_result);
    }

    SECTION("hex") {
        string query = "* a *";
        auto const query_logtypes = Grep::generate_query_substring_interpretations(query, lexer);
        set<QueryInterpretation> expected_result;
        // "* a *"
        // TODO: Because substring "* a *" matches no variable, one possible subquery logtype is
        // all static text. However, we know that if at least one of the other logtypes contains
        // a non-wildcard variable, then there is no way this query matches all static text. This
        // can also be extended to wildcard variables, for example "*10000" must match either
        // int or has#, but this has to be handled carefully as "*a" could match a variale, but
        // could also be static-text.
        QueryInterpretation query_interpretation;
        query_interpretation.append_static_token("* a *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        // "* <hex>(a) *"
        query_interpretation.clear();
        query_interpretation.append_static_token("* ");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["hex"]),
                "a",
                false,
                false
        );
        query_interpretation.append_static_token(" *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        REQUIRE(query_logtypes == expected_result);
    }

    SECTION("int") {
        string query = "* 1 *";
        auto const query_logtypes = Grep::generate_query_substring_interpretations(query, lexer);
        set<QueryInterpretation> expected_result;
        // "* 1 *"
        QueryInterpretation query_interpretation;
        query_interpretation.append_static_token("* 1 *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        // "* <int>(1) *"
        query_interpretation.clear();
        query_interpretation.append_static_token("* ");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["int"]),
                "1",
                false,
                false
        );
        query_interpretation.append_static_token(" *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        REQUIRE(query_logtypes == expected_result);
    }

    SECTION("Simple query") {
        string query = "* 10000 reply: *";
        auto const query_logtypes = Grep::generate_query_substring_interpretations(query, lexer);
        set<QueryInterpretation> expected_result;
        // "* 10000 reply: *"
        QueryInterpretation query_interpretation;
        query_interpretation.append_static_token("* 10000 reply: *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        // "* <int>(10000) reply: *"
        query_interpretation.clear();
        query_interpretation.append_static_token("* ");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["int"]),
                "10000",
                false,
                false
        );
        query_interpretation.append_static_token(" reply: *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        REQUIRE(query_logtypes == expected_result);
    }

    SECTION("Wildcard variable") {
        string query = "* *10000 *";
        auto const query_logtypes = Grep::generate_query_substring_interpretations(query, lexer);
        set<QueryInterpretation> expected_result;
        // "* *10000 *"
        QueryInterpretation query_interpretation;
        query_interpretation.append_static_token("* *10000 *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        // "*<timestamp>(* *)*10000 *"
        query_interpretation.clear();
        query_interpretation.append_static_token("*");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["timestamp"]),
                "* *",
                true,
                false
        );
        query_interpretation.append_static_token("*10000 *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        // "* *<int>(*10000) *"
        query_interpretation.clear();
        query_interpretation.append_static_token("* *");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["int"]),
                "*10000",
                true,
                false
        );
        query_interpretation.append_static_token(" *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        // "* *<int>(*10000) *" encoded
        query_interpretation.clear();
        query_interpretation.append_static_token("* *");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["int"]),
                "*10000",
                true,
                true
        );
        query_interpretation.append_static_token(" *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        // "* *<float>(*10000) *"
        query_interpretation.clear();
        query_interpretation.append_static_token("* *");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["float"]),
                "*10000",
                true,
                false
        );
        query_interpretation.append_static_token(" *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        // "* *<float>(*10000) *" encoded
        query_interpretation.clear();
        query_interpretation.append_static_token("* *");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["float"]),
                "*10000",
                true,
                true
        );
        query_interpretation.append_static_token(" *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        // "* *<hasNumber>(*10000) *"
        query_interpretation.clear();
        query_interpretation.append_static_token("* *");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["hasNumber"]),
                "*10000",
                true,
                false
        );
        query_interpretation.append_static_token(" *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        // "*timestamp(* *)*<int>(*10000) *"
        query_interpretation.clear();
        query_interpretation.append_static_token("*");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["timestamp"]),
                "* *",
                true,
                false
        );
        query_interpretation.append_static_token("*");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["int"]),
                "*10000",
                true,
                false
        );
        query_interpretation.append_static_token(" *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        // "*timestamp(* *)*<int>(*10000) *" encoded
        query_interpretation.clear();
        query_interpretation.append_static_token("*");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["timestamp"]),
                "* *",
                true,
                false
        );
        query_interpretation.append_static_token("*");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["int"]),
                "*10000",
                true,
                true
        );
        query_interpretation.append_static_token(" *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        // "*timestamp(* *)*<float>(*10000) *"
        query_interpretation.clear();
        query_interpretation.append_static_token("*");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["timestamp"]),
                "* *",
                true,
                false
        );
        query_interpretation.append_static_token("*");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["float"]),
                "*10000",
                true,
                false
        );
        query_interpretation.append_static_token(" *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        // "*timestamp(* *)*<float>(*10000) *" encoded
        query_interpretation.clear();
        query_interpretation.append_static_token("*");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["timestamp"]),
                "* *",
                true,
                false
        );
        query_interpretation.append_static_token("*");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["float"]),
                "*10000",
                true,
                true
        );
        query_interpretation.append_static_token(" *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        // "*timestamp(* *)*<hasNumber>(*10000) *"
        query_interpretation.clear();
        query_interpretation.append_static_token("*");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["timestamp"]),
                "* *",
                true,
                false
        );
        query_interpretation.append_static_token("*");
        query_interpretation.append_variable_token(
                static_cast<int>(lexer.m_symbol_id["hasNumber"]),
                "*10000",
                true,
                false
        );
        query_interpretation.append_static_token(" *");
        query_interpretation.generate_logtype_string(lexer);
        expected_result.insert(query_interpretation);
        REQUIRE(query_logtypes == expected_result);
    }
}
