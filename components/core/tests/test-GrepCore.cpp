#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <log_surgeon/log_surgeon.hpp>

#include <clp/Defs.h>
#include <clp/GrepCore.hpp>
#include <clp/Utils.hpp>

#include "search_test_utils.hpp"

using clp::epochtime_t;
using clp::GrepCore;
using clp::load_parser_from_rule_text;
using log_surgeon::ParserHandle;
using std::pair;
using std::string;
using std::vector;

namespace {
/**
 * Initializes a `ParserHandle` with space as a delimiter and the given `schema_rules`.
 *
 * @param schema_rules A vector of strings, each string representing a schema rule.
 * @return The initialized `ParserHandle`.
 */
auto make_test_parser(vector<string> const& schema_rules) -> ParserHandle;

auto make_test_parser(vector<string> const& schema_rules) -> ParserHandle {
    std::string rule_set_string{"delimiters: \\r\\n\n"};
    for (auto const& schema_rule : schema_rules) {
        rule_set_string += schema_rule + "\n";
    }

    CAPTURE(rule_set_string);

    return load_parser_from_rule_text(rule_set_string);
}
}  // namespace

TEST_CASE("get_bounds_of_next_potential_var", "[get_bounds_of_next_potential_var]") {
    string str;
    size_t begin_pos{};
    size_t end_pos{};
    bool is_var{};

    // m_end_pos past the end of the string
    str = "";
    begin_pos = string::npos;
    end_pos = string::npos;
    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == false);

    // Empty string
    str = "";
    begin_pos = 0;
    end_pos = 0;
    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == false);

    // No tokens
    str = "=";
    begin_pos = 0;
    end_pos = 0;
    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == false);

    // No wildcards
    str = " MAC address 95: ad ff 95 24 0d ff =-abc- ";
    begin_pos = 0;
    end_pos = 0;

    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("95" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("ad" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("ff" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("95" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("24" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("0d" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("ff" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE("-abc-" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(true == is_var);

    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == false);
    REQUIRE(str.length() == begin_pos);

    // With wildcards
    str = "~=1\\*x\\?!abc*123;1.2%x:+394/-=-*abc-";
    begin_pos = 0;
    end_pos = 0;

    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "1");
    REQUIRE(is_var == true);

    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "abc*123");
    REQUIRE(is_var == true);

    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "1.2");
    REQUIRE(is_var == true);

    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "+394");
    REQUIRE(is_var == true);

    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == true);
    REQUIRE(str.substr(begin_pos, end_pos - begin_pos) == "-*abc-");
    REQUIRE(is_var == false);

    REQUIRE(GrepCore::get_bounds_of_next_potential_var(str, begin_pos, end_pos, is_var) == false);
}

TEST_CASE("process_raw_query", "[dfa_search]") {
    constexpr epochtime_t cNoBeginTimestamp{0};
    constexpr epochtime_t cNoEndTimestamp{0};
    constexpr bool cIgnoreCase{true};

    auto parser{make_test_parser(
            {{R"(int:(\d+))"}, {R"(float:(\d+\.\d+))"}, {R"(hasNumber:[^ &]*\d+[^ &]*)"}}
    )};

    MockVariableDictionary const var_dict{make_var_dict({pair{0, "1a3"}, pair{1, "10a"}})};
    MockLogTypeDictionary const logtype_dict{make_logtype_dict(
            {{"text ", 'i', " ", 'i', " ", 'f'},
             {"text ", 'i', " ", 'd', " ", 'f'},
             {"text ", 'i', " ", 'd', " 3.14ab&"},
             {"text ", 'i', " ", 'd', " 3.14abc&"},
             {"text ", 'i', " ", 'd', " 3.15ab&"},
             {"text ", 'i', " 10& ", 'f'}}
    )};

    string const raw_query{"text 100 10? 3.14*"};

    auto const query{GrepCore::process_raw_query(
            logtype_dict,
            var_dict,
            raw_query,
            cNoBeginTimestamp,
            cNoEndTimestamp,
            cIgnoreCase,
            &parser
    )};

    auto const interpretations{parser.query_interpretations("", raw_query)};
    CAPTURE(interpretations.size());
    for (auto const& interpretation : interpretations) {
        string interp_string;
        for (auto const& token : interpretation) {
            if (token.qualified_name.empty()) {
                interp_string += token.value;
                continue;
            }
            interp_string += "<" + token.qualified_name + ">(" + token.value + ")";
        }
        CAPTURE(interp_string);
    }

    REQUIRE(query.has_value());
    auto const& sub_queries{query.value().get_sub_queries()};

    VarInfo const wild_int{false, true, {}};
    VarInfo const wild_has_num{true, true, {1LL}};
    size_t i{0};
    check_sub_query(i++, sub_queries, true, {wild_int, wild_has_num}, {1LL});
    check_sub_query(i++, sub_queries, true, {wild_int}, {0LL});
    check_sub_query(i++, sub_queries, true, {wild_int, wild_has_num}, {2LL, 3LL});
    check_sub_query(i++, sub_queries, true, {wild_int}, {5LL});
    REQUIRE(4 == sub_queries.size());
}
