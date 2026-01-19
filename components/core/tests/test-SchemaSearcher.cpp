#include <cstddef>
#include <cstdint>
#include <set>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <log_surgeon/Constants.hpp>
#include <log_surgeon/wildcard_query_parser/QueryInterpretation.hpp>

#include <clp/Query.hpp>
#include <clp/SchemaSearcher.hpp>

#include "SchemaSearcherTest.hpp"
#include "search_test_utils.hpp"

using clp::SubQuery;
using log_surgeon::SymbolId::TokenFloat;
using log_surgeon::SymbolId::TokenInt;
using log_surgeon::wildcard_query_parser::QueryInterpretation;
using log_surgeon::wildcard_query_parser::VariableQueryToken;
using std::pair;
using std::set;
using std::string;
using std::unordered_set;
using std::variant;
using std::vector;

constexpr uint32_t cIntId{static_cast<uint32_t>(TokenInt)};
constexpr uint32_t cFloatId{static_cast<uint32_t>(TokenFloat)};
constexpr uint32_t cHasNumId{111};

namespace {
/**
 * Constructs a `QueryInterpretation` from a vector of tokens.
 *
 * Each token is either:
 * - a `string` representing a static substring, or
 * - a `pair<uint32_t, string>`, representing a variable placeholder and its value.
 *
 * This method automatically detects whether a variable token contains a
 * wildcard (`*` or `?`).
 *
 * @param tokens Vector of tokens to populate the `QueryInterpretation`.
 * @return A `QueryInterpretation` populated with the given tokens.
 */
auto make_query_interpretation(vector<variant<string, pair<uint32_t, string>>> const& tokens)
        -> QueryInterpretation;

auto make_query_interpretation(vector<variant<string, pair<uint32_t, string>>> const& tokens)
        -> QueryInterpretation {
    QueryInterpretation interp;
    for (auto const& token : tokens) {
        if (std::holds_alternative<string>(token)) {
            interp.append_static_token(get<string>(token));
        } else {
            auto const& [symbol, value]{get<pair<uint32_t, string>>(token)};
            auto const contains_wildcard{value.find_first_of("*?") != string::npos};
            interp.append_variable_token(symbol, value, contains_wildcard);
        }
    }
    return interp;
}
}  //  namespace

TEST_CASE("get_wildcard_encodable_positions_for_empty_interpretation", "[dfa_search]") {
    QueryInterpretation const interpretation{};

    auto const positions{clp::SchemaSearcherTest::get_wildcard_encodable_positions(interpretation)};
    REQUIRE(positions.empty());
}

TEST_CASE("get_wildcard_encodable_positions_for_multi_variable_interpretation", "[dfa_search]") {
    auto const interpretation{make_query_interpretation(
            {"text",
             pair{cIntId, "100"},
             pair{cFloatId, "32.2"},
             pair{cIntId, "10?"},
             pair{cFloatId, "3.14*"},
             pair{cHasNumId, "3.14*"}}
    )};

    auto const positions{clp::SchemaSearcherTest::get_wildcard_encodable_positions(interpretation)};
    REQUIRE(2 == positions.size());
    REQUIRE(3 == positions[0]);
    REQUIRE(4 == positions[1]);
}

TEST_CASE("generate_logtype_string_for_empty_interpretation", "[dfa_search]") {
    QueryInterpretation const interpretation{};

    auto const wildcard_encodable_positions{
            clp::SchemaSearcherTest::get_wildcard_encodable_positions(interpretation)
    };

    REQUIRE(wildcard_encodable_positions.empty());
    auto const logtype_string{clp::SchemaSearcherTest::generate_logtype_string(
            interpretation,
            wildcard_encodable_positions,
            {}
    )};
    REQUIRE(logtype_string.empty());
}

TEST_CASE("generate_logtype_string_for_single_variable_interpretation", "[dfa_search]") {
    auto const expected_logtype_string{generate_expected_logtype_string({'i'})};

    auto const interpretation{make_query_interpretation({pair{cIntId, "100"}})};

    auto const wildcard_encodable_positions{
            clp::SchemaSearcherTest::get_wildcard_encodable_positions(interpretation)
    };

    REQUIRE(wildcard_encodable_positions.empty());
    auto const logtype_string{clp::SchemaSearcherTest::generate_logtype_string(
            interpretation,
            wildcard_encodable_positions,
            {false}
    )};
    REQUIRE(expected_logtype_string == logtype_string);
}

TEST_CASE("generate_logtype_string_for_multi_variable_interpretation", "[dfa_search]") {
    unordered_set<string> const expected_logtype_strings{
            generate_expected_logtype_string({"text", 'i', 'f', 'd', 'd', 'd'}),
            generate_expected_logtype_string({"text", 'i', 'f', 'i', 'd', 'd'}),
            generate_expected_logtype_string({"text", 'i', 'f', 'd', 'f', 'd'}),
            generate_expected_logtype_string({"text", 'i', 'f', 'i', 'f', 'd'})
    };

    auto const interpretation{make_query_interpretation(
            {"text",
             pair{cIntId, "100"},
             pair{cFloatId, "32.2"},
             pair{cIntId, "10?"},
             pair{cFloatId, "3.14*"},
             pair{cHasNumId, "3.14*"}}
    )};

    auto const wildcard_encodable_positions{
            clp::SchemaSearcherTest::get_wildcard_encodable_positions(interpretation)
    };

    uint64_t const num_combos{1ULL << wildcard_encodable_positions.size()};
    REQUIRE(num_combos == 4);
    unordered_set<string> logtype_strings;
    for (uint64_t mask{0}; mask < num_combos; ++mask) {
        vector<bool> mask_encoded_flags(interpretation.get_logtype().size(), false);
        for (size_t i{0}; i < wildcard_encodable_positions.size(); ++i) {
            mask_encoded_flags[wildcard_encodable_positions[i]] = (mask >> i) & 1ULL;
        }
        logtype_strings.insert(
                clp::SchemaSearcherTest::generate_logtype_string(
                        interpretation,
                        wildcard_encodable_positions,
                        mask_encoded_flags
                )
        );
    }
    REQUIRE(expected_logtype_strings == logtype_strings);
}

TEST_CASE("process_schema_empty_token", "[dfa_search]") {
    MockVariableDictionary const var_dict{make_var_dict({pair{0, "100"}})};

    SubQuery sub_query;
    VariableQueryToken const empty_int_token{cIntId, "", false};
    REQUIRE(false == clp::SchemaSearcherTest::process_token(empty_int_token, var_dict, sub_query));
    REQUIRE(false == sub_query.wildcard_match_required());
    REQUIRE(0 == sub_query.get_num_possible_vars());
}

TEST_CASE("process_schema_unmatched_token", "[dfa_search]") {
    MockVariableDictionary const var_dict{make_var_dict({pair{0, "100"}})};

    SubQuery sub_query;
    VariableQueryToken const int_token{cIntId, "200", false};
    REQUIRE(clp::SchemaSearcherTest::process_token(int_token, var_dict, sub_query));
    REQUIRE(false == sub_query.wildcard_match_required());
    REQUIRE(1 == sub_query.get_num_possible_vars());
    auto const& var{sub_query.get_vars()[0]};
    REQUIRE(false == var.is_dict_var());
    REQUIRE(var.is_precise_var());
    REQUIRE(var.get_possible_var_dict_ids().empty());
}

TEST_CASE("process_schema_int_token", "[dfa_search]") {
    MockVariableDictionary const var_dict{make_var_dict({pair{0, "100"}})};

    SubQuery sub_query;
    VariableQueryToken const int_token{cIntId, "100", false};
    REQUIRE(clp::SchemaSearcherTest::process_token(int_token, var_dict, sub_query));
    REQUIRE(false == sub_query.wildcard_match_required());
    REQUIRE(1 == sub_query.get_num_possible_vars());
    auto const& var{sub_query.get_vars()[0]};
    REQUIRE(false == var.is_dict_var());
    REQUIRE(var.is_precise_var());
    REQUIRE(var.get_possible_var_dict_ids().empty());
}

TEST_CASE("process_schema_encoded_non_greedy_wildcard_token", "[dfa_search]") {
    MockVariableDictionary const var_dict{make_var_dict({pair{0, "10a0"}, pair{1, "10b0"}})};

    SECTION("interpret_as_int") {
        SubQuery sub_query;
        VariableQueryToken const int_token{cIntId, "10?0", true};
        REQUIRE(clp::SchemaSearcherTest::process_encoded_token(int_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(0 == sub_query.get_num_possible_vars());
    }

    SECTION("interpret_as_float") {
        SubQuery sub_query;
        VariableQueryToken const float_token{cFloatId, "10?0", true};
        REQUIRE(clp::SchemaSearcherTest::process_encoded_token(float_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(0 == sub_query.get_num_possible_vars());
    }

    SECTION("interpret_as_precise_has_number") {
        SubQuery sub_query;
        VariableQueryToken const has_number_token{cHasNumId, "10a?", true};
        REQUIRE(clp::SchemaSearcherTest::process_token(has_number_token, var_dict, sub_query));
        REQUIRE(false == sub_query.wildcard_match_required());
        REQUIRE(1 == sub_query.get_num_possible_vars());
        auto const& var{sub_query.get_vars()[0]};
        REQUIRE(var.is_dict_var());
        REQUIRE(var.is_precise_var());
        REQUIRE(0 == var.get_var_dict_id());
        REQUIRE(var.get_possible_var_dict_ids().empty());
    }

    SECTION("interpret_as_imprecise_has_number") {
        SubQuery sub_query;
        VariableQueryToken const has_number_token{cHasNumId, "10?0", true};
        REQUIRE(clp::SchemaSearcherTest::process_token(has_number_token, var_dict, sub_query));
        REQUIRE(false == sub_query.wildcard_match_required());
        REQUIRE(1 == sub_query.get_num_possible_vars());
        auto const& var{sub_query.get_vars()[0]};
        REQUIRE(var.is_dict_var());
        REQUIRE(false == var.is_precise_var());
        REQUIRE(2 == var.get_possible_var_dict_ids().size());
        for (size_t i{0}; i < var.get_possible_var_dict_ids().size(); ++i) {
            REQUIRE(var.get_possible_var_dict_ids().contains(i));
        }
    }
}

// NOTE: CLP currently treats all non-encoded variables as the same, so the below test demonstrates
// this. In the future if CLP is more sophisticated, the two sections behave differently.
TEST_CASE("process_schema_non_encoded_non_greedy_wildcard_token", "[dfa_search]") {
    size_t id{0};
    MockVariableDictionary const var_dict{make_var_dict(
            {pair{id++, "100000000000000000000000010"},
             pair{id++, "100000000000000000000000020"},
             pair{id++, "100000000000000000000000030"},
             pair{id++, "1000000000000000000000000.0"},
             pair{id++, "1000000000000000000000000a0"}}
    )};

    SECTION("interpret_as_int") {
        SubQuery sub_query;
        VariableQueryToken const int_token{cIntId, "1000000000000000000000000?0", true};
        REQUIRE(clp::SchemaSearcherTest::process_token(int_token, var_dict, sub_query));
        REQUIRE(false == sub_query.wildcard_match_required());
        REQUIRE(1 == sub_query.get_num_possible_vars());
        auto const& var{sub_query.get_vars()[0]};
        REQUIRE(var.is_dict_var());
        REQUIRE(false == var.is_precise_var());
        REQUIRE(5 == var.get_possible_var_dict_ids().size());
        for (size_t i{0}; i < var.get_possible_var_dict_ids().size(); ++i) {
            REQUIRE(var.get_possible_var_dict_ids().contains(i));
        }
    }

    SECTION("interpret_as_float") {
        SubQuery sub_query;
        VariableQueryToken const float_token{cFloatId, "1000000000000000000000000?0", true};
        REQUIRE(clp::SchemaSearcherTest::process_token(float_token, var_dict, sub_query));
        REQUIRE(false == sub_query.wildcard_match_required());
        REQUIRE(1 == sub_query.get_num_possible_vars());
        auto const& var{sub_query.get_vars()[0]};
        REQUIRE(var.is_dict_var());
        REQUIRE(false == var.is_precise_var());
        REQUIRE(5 == var.get_possible_var_dict_ids().size());
        for (size_t i{0}; i < var.get_possible_var_dict_ids().size(); ++i) {
            REQUIRE(var.get_possible_var_dict_ids().contains(i));
        }
    }

    SECTION("interpret_as_has_number") {
        SubQuery sub_query;
        VariableQueryToken const has_number_token{cHasNumId, "1000000000000000000000000?0", true};
        REQUIRE(clp::SchemaSearcherTest::process_token(has_number_token, var_dict, sub_query));
        REQUIRE(false == sub_query.wildcard_match_required());
        REQUIRE(1 == sub_query.get_num_possible_vars());
        auto const& var{sub_query.get_vars()[0]};
        REQUIRE(var.is_dict_var());
        REQUIRE(false == var.is_precise_var());
        REQUIRE(5 == var.get_possible_var_dict_ids().size());
        for (size_t i{0}; i < var.get_possible_var_dict_ids().size(); ++i) {
            REQUIRE(var.get_possible_var_dict_ids().contains(i));
        }
    }
}

TEST_CASE("process_schema_greedy_wildcard_token", "[dfa_search]") {
    size_t id{0};
    MockVariableDictionary const var_dict{make_var_dict(
            {pair{id++, "10a0"},
             pair{id++, "10b0"},
             pair{id++, "100000000000000000000000010"},
             pair{id++, "100000000000000000000000020"},
             pair{id++, "100000000000000000000000030"},
             pair{id++, "1000000000000000000000000.0"},
             pair{id++, "1000000000000000000000000a0"}}
    )};

    SECTION("interpret_as_non_encoded_int") {
        SubQuery sub_query;
        VariableQueryToken const int_token{cIntId, "10*0", true};
        REQUIRE(clp::SchemaSearcherTest::process_token(int_token, var_dict, sub_query));
        REQUIRE(false == sub_query.wildcard_match_required());
        REQUIRE(1 == sub_query.get_num_possible_vars());
        auto const& var{sub_query.get_vars()[0]};
        REQUIRE(var.is_dict_var());
        REQUIRE(false == var.is_precise_var());
        REQUIRE(7 == var.get_possible_var_dict_ids().size());
        for (size_t i{0}; i < var.get_possible_var_dict_ids().size(); ++i) {
            REQUIRE(var.get_possible_var_dict_ids().contains(i));
        }
    }

    SECTION("interpret_as_non_encoded_float") {
        SubQuery sub_query;
        VariableQueryToken const float_token{cFloatId, "10*0", true};
        REQUIRE(clp::SchemaSearcherTest::process_token(float_token, var_dict, sub_query));
        REQUIRE(false == sub_query.wildcard_match_required());
        REQUIRE(1 == sub_query.get_num_possible_vars());
        auto const& var{sub_query.get_vars()[0]};
        REQUIRE(var.is_dict_var());
        REQUIRE(false == var.is_precise_var());
        REQUIRE(7 == var.get_possible_var_dict_ids().size());
        for (size_t i{0}; i < var.get_possible_var_dict_ids().size(); ++i) {
            REQUIRE(var.get_possible_var_dict_ids().contains(i));
        }
    }

    SECTION("interpret_as_non_encoded_imprecise_has_number") {
        SubQuery sub_query;
        VariableQueryToken const has_number_token{cHasNumId, "10*0", true};
        REQUIRE(clp::SchemaSearcherTest::process_token(has_number_token, var_dict, sub_query));
        REQUIRE(false == sub_query.wildcard_match_required());
        REQUIRE(1 == sub_query.get_num_possible_vars());
        auto const& var{sub_query.get_vars()[0]};
        REQUIRE(var.is_dict_var());
        REQUIRE(false == var.is_precise_var());
        REQUIRE(7 == var.get_possible_var_dict_ids().size());
        for (size_t i{0}; i < var.get_possible_var_dict_ids().size(); ++i) {
            REQUIRE(var.get_possible_var_dict_ids().contains(i));
        }
    }

    SECTION("interpret_as_non_encoded_precise_has_number") {
        SubQuery sub_query;
        VariableQueryToken const has_number_token{cHasNumId, "10b*", true};
        REQUIRE(clp::SchemaSearcherTest::process_token(has_number_token, var_dict, sub_query));
        REQUIRE(false == sub_query.wildcard_match_required());
        REQUIRE(1 == sub_query.get_num_possible_vars());
        auto const& var{sub_query.get_vars()[0]};
        REQUIRE(var.is_dict_var());
        REQUIRE(var.is_precise_var());
        REQUIRE(1 == var.get_var_dict_id());
        REQUIRE(var.get_possible_var_dict_ids().empty());
    }

    SECTION("interpret_as_encoded_int") {
        SubQuery sub_query;
        VariableQueryToken const int_token{cIntId, "10*0", true};
        REQUIRE(clp::SchemaSearcherTest::process_encoded_token(int_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(0 == sub_query.get_num_possible_vars());
    }

    SECTION("interpret_as_encoded_float") {
        SubQuery sub_query;
        VariableQueryToken const float_token{cFloatId, "10*0", true};
        REQUIRE(clp::SchemaSearcherTest::process_encoded_token(float_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(0 == sub_query.get_num_possible_vars());
    }
}

TEST_CASE("generate_schema_sub_queries", "[dfa_search]") {
    MockVariableDictionary const var_dict{
            make_var_dict({pair{0, "1a3"}, pair{1, "10a"}, pair{2, "10b"}})
    };
    MockLogTypeDictionary const logtype_dict{make_logtype_dict(
            {{"text ", 'i', " ", 'i', " ", 'f'},
             {"text ", 'i', " ", 'd', " ", 'f'},
             {"text ", 'i', " ", 'd', " 3.14ab$"},
             {"text ", 'i', " ", 'd', " 3.14abc$"},
             {"text ", 'i', " ", 'd', " 3.15ab$"},
             {"text ", 'i', " 10$ ", 'f'}}
    )};

    using V = pair<uint32_t, string>;
    vector<vector<variant<string, V>>> raw_interpretations{
            {"text ", V{cIntId, "100"}, " ", V{cIntId, "10?"}, " ", V{cFloatId, " 3.14*"}},
            {"text ", V{cIntId, "100"}, " ", V{cIntId, "10?"}, " ", V{cHasNumId, "3.14*"}},
            {"text ", V{cIntId, "100"}, " ", V{cIntId, "10?"}, " 3.14*"},
            {"text ", V{cIntId, "100"}, " ", V{cHasNumId, "10?"}, " ", V{cFloatId, " 3.14*"}},
            {"text ", V{cIntId, "100"}, " ", V{cHasNumId, "10?"}, " ", V{cHasNumId, "3.14*"}},
            {"text ", V{cIntId, "100"}, " ", V{cHasNumId, "10?"}, " 3.14*"},
            {"text ", V{cIntId, "100"}, " 10? ", V{cFloatId, " 3.14*"}},
            {"text ", V{cIntId, "100"}, " 10? ", V{cHasNumId, "3.14*"}},
            {"text ", V{cIntId, "100"}, " 10? 3.14*"}
    };
    set<QueryInterpretation> interpretations;
    for (auto const& raw_interpretation : raw_interpretations) {
        interpretations.insert(make_query_interpretation(raw_interpretation));
    }

    auto const sub_queries{clp::SchemaSearcherTest::generate_schema_sub_queries(
            interpretations,
            logtype_dict,
            var_dict
    )};

    VarInfo const wild_int{false, true, {}};
    VarInfo const wild_has_num{true, false, {1LL, 2LL}};
    REQUIRE(4 == sub_queries.size());
    size_t i{0};
    check_sub_query(i++, sub_queries, true, {wild_int, wild_has_num}, {1LL});
    check_sub_query(i++, sub_queries, true, {wild_int}, {0LL});
    check_sub_query(i++, sub_queries, false, {wild_int, wild_has_num}, {2LL, 3LL});
    check_sub_query(i++, sub_queries, true, {wild_int}, {5LL});
}

TEST_CASE("generate_schema_sub_queries_with_wildcard_duplication", "[dfa_search]") {
    MockVariableDictionary const var_dict{make_var_dict({pair{0, "1a3"}, pair{1, "10a"}})};
    MockLogTypeDictionary const logtype_dict{make_logtype_dict(
            {{"text ", 'i', " ", 'i', " ", 'f'},
             {"text ", 'i', " ", 'd', " ", 'f'},
             {"text ", 'i', " ", 'd', " 3.14ab$"},
             {"text ", 'i', " ", 'd', " 3.14abc$"},
             {"text ", 'i', " ", 'd', " 3.15ab$"},
             {"text ", 'i', " 10$ ", 'f'}}
    )};

    using V = pair<uint32_t, string>;
    vector<vector<variant<string, V>>> raw_interpretations{
            {"text ", V{cIntId, "100"}, " ", V{cIntId, "10?"}, " ", V{cFloatId, " 3.14*"}, "*"},
            {"text ", V{cIntId, "100"}, " ", V{cIntId, "10?"}, " ", V{cHasNumId, "3.14*"}, "*"},
            {"text ", V{cIntId, "100"}, " ", V{cIntId, "10?"}, " 3.14**"},
            {"text ", V{cIntId, "100"}, " ", V{cHasNumId, "10?"}, " ", V{cFloatId, " 3.14*"}, "*"},
            {"text ", V{cIntId, "100"}, " ", V{cHasNumId, "10?"}, " ", V{cHasNumId, "3.14*"}, "*"},
            {"text ", V{cIntId, "100"}, " ", V{cHasNumId, "10?"}, " 3.14**"},
            {"text ", V{cIntId, "100"}, " 10? ", V{cFloatId, " 3.14*"}, "*"},
            {"text ", V{cIntId, "100"}, " 10? ", V{cHasNumId, "3.14*"}, "*"},
            {"text ", V{cIntId, "100"}, " 10? 3.14**"}
    };
    set<QueryInterpretation> interpretations;
    for (auto const& raw_interpretation : raw_interpretations) {
        interpretations.insert(make_query_interpretation(raw_interpretation));
    }
    auto const normalized_interpretations{
            clp::SchemaSearcherTest::normalize_interpretations(interpretations)
    };

    auto const sub_queries{clp::SchemaSearcherTest::generate_schema_sub_queries(
            normalized_interpretations,
            logtype_dict,
            var_dict
    )};

    VarInfo const wild_int{false, true, {}};
    VarInfo const wild_has_num{true, true, {1LL}};
    REQUIRE(4 == sub_queries.size());
    size_t i{0};
    check_sub_query(i++, sub_queries, true, {wild_int, wild_has_num}, {1LL});
    check_sub_query(i++, sub_queries, true, {wild_int}, {0LL});
    check_sub_query(i++, sub_queries, false, {wild_int, wild_has_num}, {2LL, 3LL});
    check_sub_query(i++, sub_queries, true, {wild_int}, {5LL});
}
