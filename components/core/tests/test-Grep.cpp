#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>

#include <Catch2/single_include/catch2/catch.hpp>
#include <fmt/core.h>
#include <log_surgeon/Lexer.hpp>
#include <log_surgeon/SchemaParser.hpp>

#include "../src/clp/Grep.hpp"
#include "../src/clp/ir/types.hpp"
#include "../src/clp/QueryInterpretation.hpp"
#include "../src/clp/type_utils.hpp"
#include "log_surgeon/LogParser.hpp"

using clp::enum_to_underlying_type;
using clp::Grep;
using clp::ir::VariablePlaceholder;
using clp::load_lexer_from_file;
using clp::QueryInterpretation;
using clp::WildcardExpression;
using clp::WildcardExpressionView;
using fmt::format;
using fmt::join;
using fmt::make_format_args;
using fmt::vformat;
using log_surgeon::DelimiterStringAST;
using log_surgeon::lexers::ByteLexer;
using log_surgeon::ParserAST;
using log_surgeon::SchemaAST;
using log_surgeon::SchemaParser;
using log_surgeon::SchemaVarAST;
using std::apply;
using std::back_inserter;
using std::forward;
using std::index_sequence;
using std::make_index_sequence;
using std::make_tuple;
using std::ostream;
using std::ranges::transform;
using std::set;
using std::size_t;
using std::string;
using std::tuple;
using std::unordered_map;
using std::vector;

class ExpectedInterpretationBuilder {
public:
    explicit ExpectedInterpretationBuilder(ByteLexer& lexer) : lexer(lexer) {}

    static auto get_placeholder(string const& variable_type_name) {
        if (variable_type_name == "int") {
            return enum_to_underlying_type(VariablePlaceholder::Integer);
        }
        if (variable_type_name == "float") {
            return enum_to_underlying_type(VariablePlaceholder::Float);
        }
        return enum_to_underlying_type(VariablePlaceholder::Dictionary);
    }

    static auto get_placeholder(
            string const& variable_type_name,
            bool const force_add_to_dictionary
    ) -> uint32_t {
        if (force_add_to_dictionary) {
            return enum_to_underlying_type(VariablePlaceholder::Dictionary);
        }
        return get_placeholder(variable_type_name);
    }

    [[nodiscard]] auto build(
            string const& logtype,
            string const& has_wildcard,
            string const& is_encoded_with_wildcard,
            string const& logtype_string
    ) -> string {
        return format(
                "logtype='{}', has_wildcard='{}', is_encoded_with_wildcard='{}', "
                "logtype_string='{}'",
                logtype,
                has_wildcard,
                is_encoded_with_wildcard,
                logtype_string
        );
    }

    template <typename... VariableTypeNames>
    [[nodiscard]] auto
    build(string const& logtype,
          string const& has_wildcard,
          string const& is_encoded_with_wildcard,
          string const& logtype_string,
          VariableTypeNames const&... variable_type_names) -> string {
        auto formatted_logtype
                = vformat(logtype, make_format_args(lexer.m_symbol_id[variable_type_names]...));
        auto formatted_logtype_string = vformat(
                logtype_string,
                make_format_args(get_placeholder(variable_type_names...))
        );
        return build(
                formatted_logtype,
                has_wildcard,
                is_encoded_with_wildcard,
                formatted_logtype_string
        );
    }

    template <typename... VariableTypeNames, typename... ForceAddToDictionary>
    [[nodiscard]] auto build_verbose(
            string const& logtype,
            string const& has_wildcard,
            string const& is_encoded_with_wildcard,
            string const& logtype_string,
            VariableTypeNames const&... variable_type_names,
            ForceAddToDictionary const&... force_add_to_dictionary
    ) -> string {
        if (0 < sizeof...(force_add_to_dictionary)) {
            REQUIRE(sizeof...(variable_type_names) == sizeof...(force_add_to_dictionary));
        }

        auto formatted_logtype
                = vformat(logtype, make_format_args(lexer.m_symbol_id[variable_type_names]...));
        auto formatted_logtype_string = vformat(
                logtype_string,
                make_format_args(get_placeholder(variable_type_names..., force_add_to_dictionary...)
                )
        );
        return build(
                formatted_logtype,
                has_wildcard,
                is_encoded_with_wildcard,
                formatted_logtype_string
        );
    }

private:
    ByteLexer& lexer;
};

TEST_CASE("get_bounds_of_next_potential_var", "[get_bounds_of_next_potential_var]") {
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

TEST_CASE("SearchString", "[SearchString][schema_search]") {
    ByteLexer lexer;
    load_lexer_from_file("../tests/test_schema_files/search_schema.txt", false, lexer);

    WildcardExpression const search_string("* test\\* *");
    REQUIRE(search_string.substr(0, search_string.length()) == "* test\\* *");
    for (uint32_t idx = 0; idx < search_string.length(); idx++) {
        CAPTURE(idx);
        if (idx == 6) {
            REQUIRE(search_string.char_is_escape(idx));
        } else {
            REQUIRE(false == search_string.char_is_escape(idx));
        }
    }

    SECTION("surrounded_by_delims_or_wildcards and starts_or_ends_with_greedy_wildcard") {
        auto search_string_view1 = WildcardExpressionView{search_string, 0, search_string.length()};
        REQUIRE(search_string_view1.surrounded_by_delims_or_wildcards(lexer));
        REQUIRE(search_string_view1.starts_or_ends_with_greedy_wildcard());
        auto search_string_view2 = WildcardExpressionView{search_string, 1, search_string.length()};
        REQUIRE(search_string_view2.surrounded_by_delims_or_wildcards(lexer));
        REQUIRE(search_string_view2.starts_or_ends_with_greedy_wildcard());
        auto search_string_view3
                = WildcardExpressionView{search_string, 0, search_string.length() - 1};
        REQUIRE(search_string_view3.surrounded_by_delims_or_wildcards(lexer));
        REQUIRE(search_string_view3.starts_or_ends_with_greedy_wildcard());
        auto search_string_view4
                = WildcardExpressionView{search_string, 2, search_string.length() - 2};
        REQUIRE(search_string_view4.surrounded_by_delims_or_wildcards(lexer));
        REQUIRE(false == search_string_view4.starts_or_ends_with_greedy_wildcard());
        auto search_string_view5
                = WildcardExpressionView{search_string, 3, search_string.length() - 3};
        REQUIRE(false == search_string_view5.surrounded_by_delims_or_wildcards(lexer));
        REQUIRE(false == search_string_view5.starts_or_ends_with_greedy_wildcard());
        auto search_string_view6
                = WildcardExpressionView{search_string, 1, search_string.length() - 1};
        REQUIRE(search_string_view6.surrounded_by_delims_or_wildcards(lexer));
        REQUIRE(false == search_string_view6.starts_or_ends_with_greedy_wildcard());
    }

    SECTION("extend_to_adjacent_greedy_wildcards") {
        auto search_string_view
                = WildcardExpressionView{search_string, 1, search_string.length() - 1};
        REQUIRE(8 == search_string_view.length());
        auto extended_search_string_view = search_string_view.extend_to_adjacent_greedy_wildcards();
        REQUIRE(extended_search_string_view.surrounded_by_delims_or_wildcards(lexer));
        REQUIRE(10 == extended_search_string_view.length());
        REQUIRE(extended_search_string_view.get_value() == "* test\\* *");

        auto search_string_view2
                = WildcardExpressionView{search_string, 2, search_string.length() - 2};
        REQUIRE(6 == search_string_view2.length());
        auto extended_search_string_view2
                = search_string_view2.extend_to_adjacent_greedy_wildcards();
        REQUIRE(extended_search_string_view2.surrounded_by_delims_or_wildcards(lexer));
        REQUIRE(6 == extended_search_string_view2.length());
        REQUIRE(extended_search_string_view2.get_value() == "test\\*");
    }

    SECTION("getters") {
        auto search_string_view = WildcardExpressionView{search_string, 2, search_string.length()};
        REQUIRE(false == search_string_view.is_greedy_wildcard());
        REQUIRE(false == search_string_view.is_non_greedy_wildcard());
        REQUIRE('t' == search_string_view.get_char(0));
        REQUIRE(false == search_string_view.char_is_escape(0));
        REQUIRE(false == search_string_view.char_is_greedy_wildcard(0));
        REQUIRE(false == search_string_view.char_is_non_greedy_wildcard(0));
        REQUIRE('\\' == search_string_view.get_char(4));
        REQUIRE(search_string_view.char_is_escape(4));
        REQUIRE(false == search_string_view.char_is_greedy_wildcard(4));
        REQUIRE(false == search_string_view.char_is_non_greedy_wildcard(4));
        REQUIRE('*' == search_string_view.get_char(5));
        REQUIRE(false == search_string_view.char_is_escape(5));
        REQUIRE(false == search_string_view.char_is_greedy_wildcard(5));
        REQUIRE(false == search_string_view.char_is_non_greedy_wildcard(5));
        REQUIRE('*' == search_string_view.get_char(7));
        REQUIRE(false == search_string_view.char_is_escape(7));
        REQUIRE(search_string_view.char_is_greedy_wildcard(7));
        REQUIRE(false == search_string_view.char_is_non_greedy_wildcard(7));
    }

    SECTION("Greedy Wildcard") {
        auto search_string_view = WildcardExpressionView{search_string, 0, 1};
        REQUIRE(search_string_view.is_greedy_wildcard());
        REQUIRE(false == search_string_view.is_non_greedy_wildcard());
    }
}

TEST_CASE("get_matching_variable_types", "[get_matching_variable_types][schema_search]") {
    ByteLexer lexer;
    load_lexer_from_file("../tests/test_schema_files/search_schema.txt", false, lexer);

    constexpr std::string_view cWildcardExprValue("* 10000 reply: *");
    constexpr std::string_view cNumber = "10000";
    constexpr size_t cFirstGreedyWildcardIdx = cWildcardExprValue.find_first_of('*');
    constexpr size_t cLastGreedyWildcardIdx = cWildcardExprValue.find_last_of('*');
    constexpr size_t cECharIdx = cWildcardExprValue.find('e');
    constexpr size_t cNumberBeginIdx = cWildcardExprValue.find(cNumber);
    constexpr size_t cNumberEndIdx = cNumberBeginIdx + cNumber.length();
    WildcardExpression const wildcard_expr{string{cWildcardExprValue}};

    // Test all subexpressions of `wildcard_expr`
    for (uint32_t end_idx = 1; end_idx <= wildcard_expr.length(); end_idx++) {
        for (uint32_t begin_idx = 0; begin_idx < end_idx; begin_idx++) {
            auto [variable_types, contains_wildcard] = Grep::get_matching_variable_types(
                    WildcardExpressionView{wildcard_expr, begin_idx, end_idx},
                    lexer
            );

            std::set<uint32_t> expected_variable_types;
            if ((cFirstGreedyWildcardIdx == begin_idx && cFirstGreedyWildcardIdx + 1 == end_idx)
                || (cLastGreedyWildcardIdx == begin_idx && cLastGreedyWildcardIdx + 1 == end_idx))
            {
                // "*"
                expected_variable_types
                        = {lexer.m_symbol_id["timestamp"],
                           lexer.m_symbol_id["int"],
                           lexer.m_symbol_id["float"],
                           lexer.m_symbol_id["hex"],
                           lexer.m_symbol_id["hasNumber"],
                           lexer.m_symbol_id["uniqueVariable"],
                           lexer.m_symbol_id["test"]};
            } else if (cNumberBeginIdx <= begin_idx && end_idx <= cNumberEndIdx) {
                // Substrings of "10000"
                expected_variable_types
                        = {lexer.m_symbol_id["int"], lexer.m_symbol_id["hasNumber"]};
            } else if (cECharIdx == begin_idx && cECharIdx + 1 == end_idx) {
                // "e"
                expected_variable_types = {lexer.m_symbol_id["hex"]};
            }

            bool expected_contains_wildcard = false;
            if (cFirstGreedyWildcardIdx == begin_idx || cLastGreedyWildcardIdx + 1 == end_idx) {
                expected_contains_wildcard = true;
            }

            CAPTURE(wildcard_expr.substr(begin_idx, end_idx - begin_idx));
            CAPTURE(begin_idx);
            CAPTURE(end_idx);
            REQUIRE((variable_types == expected_variable_types));
            REQUIRE((contains_wildcard == expected_contains_wildcard));
        }
    }
}

TEST_CASE(
        "get_interpretations_for_whole_wildcard_expr",
        "[get_interpretations_for_whole_wildcard_expr][schema_search]"
) {
    ByteLexer lexer;
    load_lexer_from_file("../tests/test_schema_files/search_schema.txt", false, lexer);

    constexpr std::string_view cWildcardExprValue("* 10000 reply: *");
    constexpr std::string_view cNumber = "10000";
    constexpr size_t cNumberBeginIdx = cWildcardExprValue.find(cNumber);
    constexpr size_t cNumberEndIdx = cNumberBeginIdx + cNumber.length();
    WildcardExpression const wildcard_expr{string{cWildcardExprValue}};

    for (uint32_t end_idx = 1; end_idx <= wildcard_expr.length(); end_idx++) {
        for (uint32_t begin_idx = 0; begin_idx < end_idx; begin_idx++) {
            auto interpretations = Grep::get_interpretations_for_whole_wildcard_expr(
                    WildcardExpressionView{wildcard_expr, begin_idx, end_idx},
                    lexer
            );

            vector<QueryInterpretation> expected_interpretations(0);
            if (cNumberBeginIdx == begin_idx && cNumberEndIdx == end_idx) {
                QueryInterpretation expected_interpretation;
                expected_interpretation.append_variable_token(
                        static_cast<int>(lexer.m_symbol_id["int"]),
                        string{cNumber},
                        false,
                        false
                );
                expected_interpretations.emplace_back(expected_interpretation);
            } else if ((0 != begin_idx && wildcard_expr.length() != end_idx)
                       || (end_idx - begin_idx == 1))
            {
                QueryInterpretation expected_interpretation;
                for (uint32_t idx = begin_idx; idx < end_idx; idx++) {
                    expected_interpretation.append_static_token(wildcard_expr.substr(idx, 1));
                }
                expected_interpretations.emplace_back(expected_interpretation);
            }

            CAPTURE(begin_idx);
            CAPTURE(end_idx);
            REQUIRE((interpretations == expected_interpretations));
        }
    }
}

auto operator<<(ostream& os, unordered_map<uint32_t, string> const& map) -> ostream& {
    os << "{ ";
    for (auto const& [key, value] : map) {
        os << "{" << key << ": " << value << "} ";
    }
    os << "}";
    return os;
}

auto compare_interpretation_with_expected(
        string const& search_query_string,
        set<std::string> expected_interpretation_strings,
        ByteLexer& lexer
) -> void {
    WildcardExpression search_query(search_query_string);
    set<QueryInterpretation> const& query_interpretations
            = Grep::generate_query_substring_interpretations(search_query, lexer);
    std::set<std::string> actual_strings;
    for (auto const& query_logtype : query_interpretations) {
        std::ostringstream oss;
        oss << query_logtype;
        actual_strings.insert(oss.str());
    }

    // Compare element by element. If this test fails there is an error with one of the two shown
    // elements. One (or both) of the elements should either be excluded from their set or added to
    // the other.
    std::ostringstream oss;
    oss << lexer.m_id_symbol;
    CAPTURE(oss.str());
    while (false == actual_strings.empty() && false == expected_interpretation_strings.empty()) {
        auto it_actual = actual_strings.begin();
        auto it_expected = expected_interpretation_strings.begin();
        REQUIRE(*it_actual == *it_expected);

        actual_strings.erase(it_actual);
        expected_interpretation_strings.erase(it_expected);
    }

    // Make sure all the elements of both sets were used
    REQUIRE(actual_strings == expected_interpretation_strings);
}

TEST_CASE(
        "generate_query_substring_interpretations",
        "[generate_query_substring_interpretations][schema_search]"
) {
    ByteLexer lexer;
    load_lexer_from_file("../tests/test_schema_files/search_schema.txt", false, lexer);
    ExpectedInterpretationBuilder interp_builder(lexer);

    SECTION("Query with static text") {
        compare_interpretation_with_expected(
                "* z *",
                {//"* z *"
                 interp_builder.build("* z *", "0", "0", "* z *")
                },
                lexer
        );
    }
    SECTION("Query with a hex value") {
        // TODO: we shouldn't add the full static-text case when we can determine it is impossible.
        compare_interpretation_with_expected(
                "* a *",
                {// "* a *"
                 interp_builder.build("* a *", "0", "0", "* a *"),
                 // "* <hex>(a) *"
                 interp_builder.build("* <{}>(a) *", "000", "000", "* {} *", "hex")
                },
                lexer
        );
    }
    SECTION("Query with an integer") {
        compare_interpretation_with_expected(
                "* 10000 reply: *",
                {// "* 10000 reply: *"
                 interp_builder.build("* 10000 reply: *", "0", "0", "* 10000 reply: *"),
                 // "* <int>(10000) reply: *"
                 interp_builder
                         .build("* <{}>(10000) reply: *", "000", "000", "* {} reply: *", "int")
                },
                lexer
        );
    }
    SECTION("Query with a non-greedy wildcard at the start of a variable") {
        compare_interpretation_with_expected(
                "* ?10000 *",
                {// "* ?10000 *"
                 interp_builder.build("* ?10000 *", "0", "0", "* ?10000 *"),
                 // "* ?<int>(10000) *"
                 interp_builder.build("* ?<{}>(10000) *", "000", "000", "* ?{} *", "int"),
                 // "* <int>(?10000) *"
                 // TODO: Add logic to determine this case is impossible.
                 interp_builder
                         .build_verbose("* <{}>(?10000) *", "010", "000", "* {} *", "int", true),
                 interp_builder
                         .build_verbose("* <{}>(?10000) *", "010", "010", "* {} *", "int", false),
                 // "* <hasNumber>(?10000) *"
                 interp_builder.build("* <{}>(?10000) *", "010", "000", "* {} *", "hasNumber")
                },
                lexer
        );
    }
    /*
    SECTION("Query with a non-greedy wildcard at the end of a variable") {
        compare_interpretation_with_expected(
                "* 10000? *",
                {
                        // "* 10000? *"
                        interp_builder.build("* 10000? *", "0", "0", "* 10000? *"),
                        // "* <int>(10000)? *"
                        interp_builder.build("* <{}>(10000)? *", "000", "000", "* {}? *", "int"),
                        // "* <int>(10000?) *"
                        interp_builder
                                .build("* <{}>(10000?) *", "010", "000", "* {} *", "int", true),
                        interp_builder
                                .build("* <{}>(10000?) *", "010", "010", "* {} *", "int", false),
                        // "* <hasNumber>(10000?) *"
                        // interp_builder.build("* <{}>(10000?) *", "010", "000", "* {} *", {},
                        // "hasNumber")
                },
                lexer
        );
    }
    SECTION("Query with a non-greedy wildcard in the middle of a variable") {
        compare_interpretation_with_expected(
                "* 100?00 *",
                {
                        // "* 10000? *"
                        // interp_builder.build("* 100?00 *", "0", "0", "* 100?00 *", {}),
                        // "* <int>(100?00) *"
                        // interp_builder.build("* <{}>(100?00) *", "010", "000", "* {} *", {true},
                        // "int"), interp_builder.build("* <{}>(100?00) *", "010", "010", "* {} *",
                        // {false}, "int"),
                        // "* <hasNumber>(100?00) *"
                        // interp_builder.build("* <{}>(100?00) *", "010", "000", "* {} *", {},
                        // "hasNumber"),
                        // "* <hasNumber>(100?00) *"
                        // interp_builder.build("* <{}>(100?00) *", "010", "000", "* {} *", {},
                        // "hasNumber"),
                        // "* <int>(100)?00 *"
                        // TODO: Add logic to determine this case is impossible.
                        // interp_builder.build("* <{}>(100)?00 *", "000", "000", "* {}?00 *", {},
                        // "int"),
                        // "* 100?<int>(00) *"
                        // TODO: Add logic to determine this case is impossible.
                        // interp_builder
                        //        .build("* 100?<{}>(00) *", "000", "000", "* 100?{} *", {true},
                        //        "int"),
                        // "* <int>(100)?<int>(00) *"
                        // interp_builder.build(
                        //        "* <{}>(100)?<{}>(00) *",
                        //        "000",
                        //        "000",
                        //        "* {}?{} *",
                        //        {false, true},
                        //        "int",
                        //        "int"
                        //)
                },
                lexer
        );
    }
    SECTION("Query with a non-greedy wildcard and escaped wildcard") {
        compare_interpretation_with_expected(
                "* 10\\?000? *",
                {// "* 10\\?000? *"
                 format("logtype='* 10\\?000? *', has_wildcard='0', is_encoded_with_wildcard='0', "
                        "logtype_string='* 10\\?000? *'"),
                 // "* <int>(10)\?000? *"
                 format("logtype='* <{}>(10)\\?000? *', has_wildcard='000', "
                        "is_encoded_with_wildcard='000', "
                        "logtype_string='* {}\\?000? *'",
                        lexer.m_symbol_id["int"],
                        enum_to_underlying_type(VariablePlaceholder::Integer)),
                 // "* <int>(10)\?<int>(000)? *"
                 format("logtype='* <{}>(10)\\?<{}>(000)? *', has_wildcard='00000', "
                        "is_encoded_with_wildcard='00000', "
                        "logtype_string='* {}\\?{}? *'",
                        lexer.m_symbol_id["int"],
                        lexer.m_symbol_id["int"],
                        enum_to_underlying_type(VariablePlaceholder::Integer),
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "* <int>(10)\?<int>(000?) *"
                 format("logtype='* <{}>(10)\\?<{}>(000?) *', has_wildcard='00010', "
                        "is_encoded_with_wildcard='00000', "
                        "logtype_string='* {}\\?{} *'",
                        lexer.m_symbol_id["int"],
                        lexer.m_symbol_id["int"],
                        enum_to_underlying_type(VariablePlaceholder::Integer),
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "* <int>(10)\?<int>(000?) *" encoded
                 format("logtype='* <{}>(10)\\?<{}>(000?) *', has_wildcard='00010', "
                        "is_encoded_with_wildcard='00010', "
                        "logtype_string='* {}\\?{} *'",
                        lexer.m_symbol_id["int"],
                        lexer.m_symbol_id["int"],
                        enum_to_underlying_type(VariablePlaceholder::Integer),
                        enum_to_underlying_type(VariablePlaceholder::Integer)),
                 // "* <int>(10)\?<hasNumber>(000?) *"
                 format("logtype='* <{}>(10)\\?<{}>(000?) *', has_wildcard='00010', "
                        "is_encoded_with_wildcard='00000', "
                        "logtype_string='* {}\\?{} *'",
                        lexer.m_symbol_id["int"],
                        lexer.m_symbol_id["hasNumber"],
                        enum_to_underlying_type(VariablePlaceholder::Integer),
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "* 10\?<int>(000)? *"
                 format("logtype='* 10\\?<{}>(000)? *', has_wildcard='000', "
                        "is_encoded_with_wildcard='000', "
                        "logtype_string='* 10\\?{}? *'",
                        lexer.m_symbol_id["int"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "* 10\?<int>(000?) *"
                 format("logtype='* 10\\?<{}>(000?) *', has_wildcard='010', "
                        "is_encoded_with_wildcard='000', "
                        "logtype_string='* 10\\?{} *'",
                        lexer.m_symbol_id["int"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "* 10\?<hasNumber>(000?) *" encoded
                 format("logtype='* 10\\?<{}>(000?) *', has_wildcard='010', "
                        "is_encoded_with_wildcard='010', "
                        "logtype_string='* 10\\?{} *'",
                        lexer.m_symbol_id["int"],
                        enum_to_underlying_type(VariablePlaceholder::Integer)),
                 // "* 10\?<hasNumber>(000?) *" encoded
                 format("logtype='* 10\\?<{}>(000?) *', has_wildcard='010', "
                        "is_encoded_with_wildcard='000', "
                        "logtype_string='* 10\\?{} *'",
                        lexer.m_symbol_id["hasNumber"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary))
                },
                lexer
        );
    }
    SECTION("Query with greedy wildcard") {
        compare_interpretation_with_expected(
                "* *10000 *",
                {// "* *10000 *"
                 format("logtype='* *10000 *', has_wildcard='0', is_encoded_with_wildcard='0', "
                        "logtype_string='* *10000 *'"),
                 // "*<timestamp>(* *)*10000 *"
                 format("logtype='*<{}>(* *)*10000 *', has_wildcard='010', "
                        "is_encoded_with_wildcard='000', "
                        "logtype_string='*{}*10000 *'",
                        lexer.m_symbol_id["timestamp"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "* *<int>(*10000) *"
                 format("logtype='* *<{}>(*10000) *', has_wildcard='010', "
                        "is_encoded_with_wildcard='000', "
                        "logtype_string='* *{} *'",
                        lexer.m_symbol_id["int"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "* *<int>(*10000) *" encoded
                 format("logtype='* *<{}>(*10000) *', has_wildcard='010', "
                        "is_encoded_with_wildcard='010', "
                        "logtype_string='* *{} *'",
                        lexer.m_symbol_id["int"],
                        enum_to_underlying_type(VariablePlaceholder::Integer)),
                 // "* *<float>(*10000) *"
                 format("logtype='* *<{}>(*10000) *', has_wildcard='010', "
                        "is_encoded_with_wildcard='000', "
                        "logtype_string='* *{} *'",
                        lexer.m_symbol_id["float"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "* *<float>(*10000) *" encoded
                 format("logtype='* *<{}>(*10000) *', has_wildcard='010', "
                        "is_encoded_with_wildcard='010', "
                        "logtype_string='* *{} *'",
                        lexer.m_symbol_id["float"],
                        enum_to_underlying_type(VariablePlaceholder::Float)),
                 // "* *<hasNumber>(*10000) *"
                 format("logtype='* *<{}>(*10000) *', has_wildcard='010', "
                        "is_encoded_with_wildcard='000', "
                        "logtype_string='* *{} *'",
                        lexer.m_symbol_id["hasNumber"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "*<timestamp>(* *)*<int>(*10000) *"
                 format("logtype='*<{}>(* *)*<{}>(*10000) *', has_wildcard='01010', "
                        "is_encoded_with_wildcard='00000', "
                        "logtype_string='*{}*{} *'",
                        lexer.m_symbol_id["timestamp"],
                        lexer.m_symbol_id["int"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary),
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "*<timestamp>(* *)*<int>(*10000) *" encoded
                 format("logtype='*<{}>(* *)*<{}>(*10000) *', has_wildcard='01010', "
                        "is_encoded_with_wildcard='00010', "
                        "logtype_string='*{}*{} *'",
                        lexer.m_symbol_id["timestamp"],
                        lexer.m_symbol_id["int"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary),
                        enum_to_underlying_type(VariablePlaceholder::Integer)),
                 // "*<timestamp>(* *)*<float>(*10000) *"
                 format("logtype='*<{}>(* *)*<{}>(*10000) *', has_wildcard='01010', "
                        "is_encoded_with_wildcard='00000', "
                        "logtype_string='*{}*{} *'",
                        lexer.m_symbol_id["timestamp"],
                        lexer.m_symbol_id["float"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary),
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "*<timestamp>(* *)*<float>(*10000) *" encoded
                 format("logtype='*<{}>(* *)*<{}>(*10000) *', has_wildcard='01010', "
                        "is_encoded_with_wildcard='00010', "
                        "logtype_string='*{}*{} *'",
                        lexer.m_symbol_id["timestamp"],
                        lexer.m_symbol_id["float"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary),
                        enum_to_underlying_type(VariablePlaceholder::Float)),
                 // "*<timestamp>(* *)*<hasNumber>(*10000) *"
                 format("logtype='*<{}>(* *)*<{}>(*10000) *', has_wildcard='01010', "
                        "is_encoded_with_wildcard='00000', "
                        "logtype_string='*{}*{} *'",
                        lexer.m_symbol_id["timestamp"],
                        lexer.m_symbol_id["hasNumber"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary),
                        enum_to_underlying_type(VariablePlaceholder::Dictionary))
                },
                lexer
        );
    }
    SECTION("Query with greedy wildcard followed by non-greedy wildcard") {
        compare_interpretation_with_expected(
                "* *?10000 *",
                {// "* *?10000 *"
                 format("logtype='* *?10000 *', has_wildcard='0', is_encoded_with_wildcard='0', "
                        "logtype_string='* *?10000 *'"),
                 // "*<timestamp>(* *)*?10000 *"
                 format("logtype='*<{}>(* *)*?10000 *', has_wildcard='010', "
                        "is_encoded_with_wildcard='000', "
                        "logtype_string='*{}*?10000 *'",
                        lexer.m_symbol_id["timestamp"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "*<timestamp>(* *)*?10000 *"
                 format("logtype='*<{}>(* *)*?10000 *', has_wildcard='010', "
                        "is_encoded_with_wildcard='000', "
                        "logtype_string='*{}*?10000 *'",
                        lexer.m_symbol_id["timestamp"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "* *<int>(*?10000) *"
                 format("logtype='* *<{}>(*?10000) *', has_wildcard='010', "
                        "is_encoded_with_wildcard='000', "
                        "logtype_string='* *{} *'",
                        lexer.m_symbol_id["int"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "* *<int>(*?10000) *" encoded
                 format("logtype='* *<{}>(*?10000) *', has_wildcard='010', "
                        "is_encoded_with_wildcard='010', "
                        "logtype_string='* *{} *'",
                        lexer.m_symbol_id["int"],
                        enum_to_underlying_type(VariablePlaceholder::Integer)),
                 // "* *<float>(*?10000) *"
                 format("logtype='* *<{}>(*?10000) *', has_wildcard='010', "
                        "is_encoded_with_wildcard='000', "
                        "logtype_string='* *{} *'",
                        lexer.m_symbol_id["float"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "* *<float>(*?10000) *" encoded
                 format("logtype='* *<{}>(*?10000) *', has_wildcard='010', "
                        "is_encoded_with_wildcard='010', "
                        "logtype_string='* *{} *'",
                        lexer.m_symbol_id["float"],
                        enum_to_underlying_type(VariablePlaceholder::Float)),
                 // "* *<hasNumber>(*?10000) *"
                 format("logtype='* *<{}>(*?10000) *', has_wildcard='010', "
                        "is_encoded_with_wildcard='000', "
                        "logtype_string='* *{} *'",
                        lexer.m_symbol_id["hasNumber"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "*<timestamp>(* *)*<int>(*?10000) *"
                 format("logtype='*<{}>(* *)*<{}>(*?10000) *', has_wildcard='01010', "
                        "is_encoded_with_wildcard='00000', "
                        "logtype_string='*{}*{} *'",
                        lexer.m_symbol_id["timestamp"],
                        lexer.m_symbol_id["int"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary),
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "*<timestamp>(* *)*<int>(*?10000) *" encoded
                 format("logtype='*<{}>(* *)*<{}>(*?10000) *', has_wildcard='01010', "
                        "is_encoded_with_wildcard='00010', "
                        "logtype_string='*{}*{} *'",
                        lexer.m_symbol_id["timestamp"],
                        lexer.m_symbol_id["int"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary),
                        enum_to_underlying_type(VariablePlaceholder::Integer)),
                 // "*<timestamp>(* *)*<float>(*?10000) *"
                 format("logtype='*<{}>(* *)*<{}>(*?10000) *', has_wildcard='01010', "
                        "is_encoded_with_wildcard='00000', "
                        "logtype_string='*{}*{} *'",
                        lexer.m_symbol_id["timestamp"],
                        lexer.m_symbol_id["float"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary),
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "*<timestamp>(* *)*<float>(*?10000) *" encoded
                 format("logtype='*<{}>(* *)*<{}>(*?10000) *', has_wildcard='01010', "
                        "is_encoded_with_wildcard='00010', "
                        "logtype_string='*{}*{} *'",
                        lexer.m_symbol_id["timestamp"],
                        lexer.m_symbol_id["float"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary),
                        enum_to_underlying_type(VariablePlaceholder::Float)),
                 // "*<timestamp>(* *)*<hasNumber>(*?10000) *"
                 format("logtype='*<{}>(* *)*<{}>(*?10000) *', has_wildcard='01010', "
                        "is_encoded_with_wildcard='00000', "
                        "logtype_string='*{}*{} *'",
                        lexer.m_symbol_id["timestamp"],
                        lexer.m_symbol_id["hasNumber"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary),
                        enum_to_underlying_type(VariablePlaceholder::Dictionary)),
                 // "* *?<int>(10000) *"
                 format("logtype='* *?<{}>(10000) *', has_wildcard='000', "
                        "is_encoded_with_wildcard='000', "
                        "logtype_string='* *?{} *'",
                        lexer.m_symbol_id["int"],
                        enum_to_underlying_type(VariablePlaceholder::Integer)),
                 // "*<timestamp>(* *)*?<int>(10000) *"
                 format("logtype='*<{}>(* *)*?<{}>(10000) *', has_wildcard='01000', "
                        "is_encoded_with_wildcard='00000', "
                        "logtype_string='*{}*?{} *'",
                        lexer.m_symbol_id["timestamp"],
                        lexer.m_symbol_id["int"],
                        enum_to_underlying_type(VariablePlaceholder::Dictionary),
                        enum_to_underlying_type(VariablePlaceholder::Integer))
                },
                lexer
        );
    }
    */
    /*
SECTION("Query with non-greedy wildcard followed by greedy wildcard") {
    set<string> expected_interpretation_strings;
    // "* ?*10000 *"
    expected_interpretation_strings.insert(
            format("logtype='* ?*10000 *', has_wildcard='0', "
                        "is_encoded_with_wildcard='0', "
                        "logtype_string='* ?*10000 *'")
    );
    // "*<timestamp>(* *)?*10000 *"
    expected_interpretation_strings.insert(format(
            "logtype='*<{}>(* *)?*10000 *', has_wildcard='010', "
            "is_encoded_with_wildcard='000', "
            "logtype_string='*{}?*10000 *'",
            lexer.m_symbol_id["timestamp"],
            enum_to_underlying_type(VariablePlaceholder::Dictionary)
    ));
    // "* <int>(?*10000) *"
    for () {
        expected_interpretation_strings.insert(format(
                "logtype='* <{}>(?*10000) *', has_wildcard='010', "
                "is_encoded_with_wildcard='000', "
                "logtype_string='* {} *'",
                lexer.m_symbol_id["int"],
                enum_to_underlying_type(VariablePlaceholder::Dictionary)
        ));
    }

    compare_interpretation_with_expected(
            "* ?*10000 *",
            {,
             // "* *<int>(?*10000) *" encoded
             format(
                     "logtype='* <{}>(?*10000) *', has_wildcard='010', "
                     "is_encoded_with_wildcard='010', "
                     "logtype_string='* {} *'",
                     lexer.m_symbol_id["int"],
                     enum_to_underlying_type(VariablePlaceholder::Integer)
             ),
             // "* <float>(?*10000) *"
             format(
                     "logtype='* <{}>(?*10000) *', has_wildcard='010', "
                     "is_encoded_with_wildcard='000', "
                     "logtype_string='* {} *'",
                     lexer.m_symbol_id["float"],
                     enum_to_underlying_type(VariablePlaceholder::Dictionary)
             ),
             // "* <float>(?*10000) *" encoded
             format(
                     "logtype='* <{}>(?*10000) *', has_wildcard='010', "
                     "is_encoded_with_wildcard='010', "
                     "logtype_string='* {} *'",
                     lexer.m_symbol_id["float"],
                     enum_to_underlying_type(VariablePlaceholder::Float)
             ),
             // "* <hasNumber>(?*10000) *"
             format(
                     "logtype='* <{}>(?*10000) *', has_wildcard='010', "
                     "is_encoded_with_wildcard='000', "
                     "logtype_string='* {} *'",
                     lexer.m_symbol_id["hasNumber"],
                     enum_to_underlying_type(VariablePlaceholder::Dictionary)
             ),
             // "*<timestamp>(* *)*<int>(?*10000) *"
             format(
                     "logtype='*<{}>(* *)*<{}>(?*10000) *', has_wildcard='01010', "
                     "is_encoded_with_wildcard='00000', "
                     "logtype_string='*{}*{} *'",
                     lexer.m_symbol_id["timestamp"],
                     lexer.m_symbol_id["int"],
                     enum_to_underlying_type(VariablePlaceholder::Dictionary),
                     enum_to_underlying_type(VariablePlaceholder::Dictionary)
             ),
             // "*<timestamp>(* *)*<int>(?*10000) *" encoded
             format(
                     "logtype='*<{}>(* *)*<{}>(?*10000) *', has_wildcard='01010', "
                     "is_encoded_with_wildcard='00010', "
                     "logtype_string='*{}*{} *'",
                     lexer.m_symbol_id["timestamp"],
                     lexer.m_symbol_id["int"],
                     enum_to_underlying_type(VariablePlaceholder::Dictionary),
                     enum_to_underlying_type(VariablePlaceholder::Integer)
             ),
             // "*<timestamp>(* *)*<float>(?*10000) *"
             format(
                     "logtype='*<{}>(* *)*<{}>(?*10000) *', has_wildcard='01010', "
                     "is_encoded_with_wildcard='00000', "
                     "logtype_string='*{}*{} *'",
                     lexer.m_symbol_id["timestamp"],
                     lexer.m_symbol_id["float"],
                     enum_to_underlying_type(VariablePlaceholder::Dictionary),
                     enum_to_underlying_type(VariablePlaceholder::Dictionary)
             ),
             // "*<timestamp>(* *)*<float>(?*10000) *" encoded
             format(
                     "logtype='*<{}>(* *)*<{}>(?*10000) *', has_wildcard='01010', "
                     "is_encoded_with_wildcard='00010', "
                     "logtype_string='*{}*{} *'",
                     lexer.m_symbol_id["timestamp"],
                     lexer.m_symbol_id["float"],
                     enum_to_underlying_type(VariablePlaceholder::Dictionary),
                     enum_to_underlying_type(VariablePlaceholder::Float)
             ),
             // "*<timestamp>(* *)*<hasNumber>(?*10000) *"
             format(
                     "logtype='*<{}>(* *)*<{}>(?*10000) *', has_wildcard='01010', "
                     "is_encoded_with_wildcard='00000', "
                     "logtype_string='*{}*{} *'",
                     lexer.m_symbol_id["timestamp"],
                     lexer.m_symbol_id["hasNumber"],
                     enum_to_underlying_type(VariablePlaceholder::Dictionary),
                     enum_to_underlying_type(VariablePlaceholder::Dictionary)
             ),
             // "* ?*<int>(10000) *"
             format(
                     "logtype='* ?*<{}>(10000) *', has_wildcard='000', "
                     "is_encoded_with_wildcard='000', "
                     "logtype_string='* ?*{} *'",
                     lexer.m_symbol_id["int"],
                     enum_to_underlying_type(VariablePlaceholder::Integer)
             ),
             // "*<timestamp>(* *)?*<int>(10000) *"
             format(
                     "logtype='*<{}>(* ?*)*<{}>(10000) *', has_wildcard='01000', "
                     "is_encoded_with_wildcard='00000', "
                     "logtype_string='*{}*{} *'",
                     lexer.m_symbol_id["timestamp"],
                     lexer.m_symbol_id["int"],
                     enum_to_underlying_type(VariablePlaceholder::Dictionary),
                     enum_to_underlying_type(VariablePlaceholder::Integer)
             ),
             // "*<timestamp>(* *)?*<int>(10000) *"
             format(
                     "logtype='*<{}>(* ?*)*<{}>(10000) *', has_wildcard='01000', "
                     "is_encoded_with_wildcard='00000', "
                     "logtype_string='*{}*{} *'",
                     lexer.m_symbol_id["timestamp"],
                     lexer.m_symbol_id["int"],
                     enum_to_underlying_type(VariablePlaceholder::Dictionary),
                     enum_to_underlying_type(VariablePlaceholder::Integer)
             ),
             // "* <int>(*?)*10000 *"
             format(
                     "logtype='* <{}>(?*)*10000 *', has_wildcard='010', "
                     "is_encoded_with_wildcard='000', "
                     "logtype_string='* {}*10000 *'",
                     lexer.m_symbol_id["int"],
                     enum_to_underlying_type(VariablePlaceholder::Dictionary)
             ),
             // "* <int>(*?)*10000 * encoded"
             format(
                     "logtype='* <{}>(?*)*10000 *', has_wildcard='010', "
                     "is_encoded_with_wildcard='010', "
                     "logtype_string='* {}*10000 *'",
                     lexer.m_symbol_id["int"],
                     enum_to_underlying_type(VariablePlaceholder::Integer)
             ),
             // "* <int>(*?)*<int>(*10000) *" dict + dict
             format(
                     "logtype='* <{}>(?*)*<{}>(*10000) *', has_wildcard='01010', "
                     "is_encoded_with_wildcard='00000', "
                     "logtype_string='* {}*{} *'",
                     lexer.m_symbol_id["int"],
                     lexer.m_symbol_id["int"],
                     enum_to_underlying_type(VariablePlaceholder::Dictionary),
                     enum_to_underlying_type(VariablePlaceholder::Dictionary)
             ),
             // "* <int>(*?)*<int>(*10000) *"  encoded + dict
             format(
                     "logtype='* <{}>(?*)*<{}>(*10000) *', has_wildcard='01010', "
                     "is_encoded_with_wildcard='01000', "
                     "logtype_string='* {}*{} *'",
                     lexer.m_symbol_id["int"],
                     lexer.m_symbol_id["int"],
                     enum_to_underlying_type(VariablePlaceholder::Integer),
                     enum_to_underlying_type(VariablePlaceholder::Dictionary)
             ),
             // "* <int>(*?)*<int>(*10000) *"  dict + encoded
             format(
                     "logtype='* <{}>(?*)*<{}>(*10000) *', has_wildcard='01010', "
                     "is_encoded_with_wildcard='00010', "
                     "logtype_string='* {}*{} *'",
                     lexer.m_symbol_id["int"],
                     lexer.m_symbol_id["int"],
                     enum_to_underlying_type(VariablePlaceholder::Dictionary),
                     enum_to_underlying_type(VariablePlaceholder::Integer)
             ),
             // "* <int>(*?)*<int>(*10000) *" encoded + encoded
             format(
                     "logtype='* <{}>(?*)*<{}>(*10000) *', has_wildcard='01010', "
                     "is_encoded_with_wildcard='01010', "
                     "logtype_string='* {}*{} *'",
                     lexer.m_symbol_id["int"],
                     lexer.m_symbol_id["int"],
                     enum_to_underlying_type(VariablePlaceholder::Integer),
                     enum_to_underlying_type(VariablePlaceholder::Integer)
             )},
            lexer
    );
}
*/
}
