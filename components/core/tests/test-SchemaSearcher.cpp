#include <cstddef>
#include <cstdint>
#include <set>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <log_surgeon/log_surgeon.hpp>

#include <clp/Query.hpp>
#include <clp/SchemaSearcher.hpp>

#include "SchemaSearcherTest.hpp"
#include "search_test_utils.hpp"

#define S(X) log_surgeon::SubQuery("", X)

using std::pair;
using std::set;
using std::string;
using std::unordered_set;
using std::variant;
using std::vector;

std::string const c_float{"float"};
std::string const c_has_num{"hasNum"};
std::string const c_int{"int"};

TEST_CASE("get_wildcard_encodable_positions_for_empty_interpretation", "[dfa_search]") {
    std::vector<log_surgeon::SubQuery> const interpretation{};

    auto const positions{clp::SchemaSearcherTest::get_wildcard_encodable_positions(interpretation)};
    REQUIRE(positions.empty());
}

TEST_CASE("get_wildcard_encodable_positions_for_multi_variable_interpretation", "[dfa_search]") {
    std::vector<log_surgeon::SubQuery> interpretation{
            {"", "text"},
            {c_int, "100"},
            {c_float, "32.2"},
            {c_int, "10?"},
            {c_float, "3.14*"},
            {c_has_num, "3.14*"}
    };

    auto const positions{clp::SchemaSearcherTest::get_wildcard_encodable_positions(interpretation)};
    REQUIRE(2 == positions.size());
    REQUIRE(3 == positions[0]);
    REQUIRE(4 == positions[1]);
}

TEST_CASE("generate_logtype_string_for_empty_interpretation", "[dfa_search]") {
    std::vector<log_surgeon::SubQuery> const interpretation{};

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

    std::vector<log_surgeon::SubQuery> const interpretation{{c_int, "100"}};

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

    std::vector<log_surgeon::SubQuery> const interpretation{
            S("text"),
            {c_int, "100"},
            {c_float, "32.2"},
            {c_int, "10?"},
            {c_float, "3.14*"},
            {c_has_num, "3.14*"}
    };

    auto const wildcard_encodable_positions{
            clp::SchemaSearcherTest::get_wildcard_encodable_positions(interpretation)
    };

    uint64_t const num_combos{1ULL << wildcard_encodable_positions.size()};
    REQUIRE(num_combos == 4);
    unordered_set<string> logtype_strings;
    for (uint64_t mask{0}; mask < num_combos; ++mask) {
        vector<bool> mask_encoded_flags(interpretation.size(), false);
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

    clp::SubQuery sub_query;
    log_surgeon::SubQuery const empty_int_token{c_int, ""};
    REQUIRE(false == clp::SchemaSearcherTest::process_token(empty_int_token, var_dict, sub_query));
    REQUIRE(sub_query.wildcard_match_required());
    REQUIRE(0 == sub_query.get_num_possible_vars());
}

TEST_CASE("process_schema_unmatched_token", "[dfa_search]") {
    MockVariableDictionary const var_dict{make_var_dict({pair{0, "100"}})};

    clp::SubQuery sub_query;
    log_surgeon::SubQuery const int_token{c_int, "200"};
    REQUIRE(clp::SchemaSearcherTest::process_token(int_token, var_dict, sub_query));
    REQUIRE(sub_query.wildcard_match_required());
    REQUIRE(1 == sub_query.get_num_possible_vars());
    auto const& var{sub_query.get_vars()[0]};
    REQUIRE(false == var.is_dict_var());
    REQUIRE(var.is_precise_var());
    REQUIRE(var.get_possible_var_dict_ids().empty());
}

TEST_CASE("process_schema_int_token", "[dfa_search]") {
    MockVariableDictionary const var_dict{make_var_dict({pair{0, "100"}})};

    clp::SubQuery sub_query;
    log_surgeon::SubQuery const int_token{c_int, "100"};
    REQUIRE(clp::SchemaSearcherTest::process_token(int_token, var_dict, sub_query));
    REQUIRE(sub_query.wildcard_match_required());
    REQUIRE(1 == sub_query.get_num_possible_vars());
    auto const& var{sub_query.get_vars()[0]};
    REQUIRE(false == var.is_dict_var());
    REQUIRE(var.is_precise_var());
    REQUIRE(var.get_possible_var_dict_ids().empty());
}

TEST_CASE("process_schema_encoded_non_greedy_wildcard_token", "[dfa_search]") {
    MockVariableDictionary const var_dict{make_var_dict({pair{0, "10a0"}, pair{1, "10b0"}})};

    SECTION("interpret_as_int") {
        clp::SubQuery sub_query;
        log_surgeon::SubQuery const int_token{c_int, "10?0"};
        REQUIRE(clp::SchemaSearcherTest::process_encoded_token(int_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(0 == sub_query.get_num_possible_vars());
    }

    SECTION("interpret_as_float") {
        clp::SubQuery sub_query;
        log_surgeon::SubQuery const float_token{c_float, "10?0"};
        REQUIRE(clp::SchemaSearcherTest::process_encoded_token(float_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(0 == sub_query.get_num_possible_vars());
    }

    SECTION("interpret_as_precise_has_number") {
        clp::SubQuery sub_query;
        log_surgeon::SubQuery const has_number_token{c_has_num, "10a?"};
        REQUIRE(clp::SchemaSearcherTest::process_token(has_number_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(1 == sub_query.get_num_possible_vars());
        auto const& var{sub_query.get_vars()[0]};
        REQUIRE(var.is_dict_var());
        REQUIRE(var.is_precise_var());
        REQUIRE(0 == var.get_var_dict_id());
        REQUIRE(var.get_possible_var_dict_ids().empty());
    }

    SECTION("interpret_as_imprecise_has_number") {
        clp::SubQuery sub_query;
        log_surgeon::SubQuery const has_number_token{c_has_num, "10?0"};
        REQUIRE(clp::SchemaSearcherTest::process_token(has_number_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
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
        clp::SubQuery sub_query;
        log_surgeon::SubQuery const int_token{c_int, "1000000000000000000000000?0"};
        REQUIRE(clp::SchemaSearcherTest::process_token(int_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
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
        clp::SubQuery sub_query;
        log_surgeon::SubQuery const float_token{c_float, "1000000000000000000000000?0"};
        REQUIRE(clp::SchemaSearcherTest::process_token(float_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
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
        clp::SubQuery sub_query;
        log_surgeon::SubQuery const has_num_token{c_has_num, "1000000000000000000000000?0"};
        REQUIRE(clp::SchemaSearcherTest::process_token(has_num_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
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
        clp::SubQuery sub_query;
        log_surgeon::SubQuery const int_token{c_int, "10*0"};
        REQUIRE(clp::SchemaSearcherTest::process_token(int_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
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
        clp::SubQuery sub_query;
        log_surgeon::SubQuery const float_token{c_float, "10*0"};
        REQUIRE(clp::SchemaSearcherTest::process_token(float_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
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
        clp::SubQuery sub_query;
        log_surgeon::SubQuery const has_number_token{c_has_num, "10*0"};
        REQUIRE(clp::SchemaSearcherTest::process_token(has_number_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
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
        clp::SubQuery sub_query;
        log_surgeon::SubQuery const has_number_token{c_has_num, "10b*"};
        REQUIRE(clp::SchemaSearcherTest::process_token(has_number_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(1 == sub_query.get_num_possible_vars());
        auto const& var{sub_query.get_vars()[0]};
        REQUIRE(var.is_dict_var());
        REQUIRE(var.is_precise_var());
        REQUIRE(1 == var.get_var_dict_id());
        REQUIRE(var.get_possible_var_dict_ids().empty());
    }

    SECTION("interpret_as_encoded_int") {
        clp::SubQuery sub_query;
        log_surgeon::SubQuery const int_token{c_int, "10*0"};
        REQUIRE(clp::SchemaSearcherTest::process_encoded_token(int_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(0 == sub_query.get_num_possible_vars());
    }

    SECTION("interpret_as_encoded_float") {
        clp::SubQuery sub_query;
        log_surgeon::SubQuery const float_token{c_float, "10*0"};
        REQUIRE(clp::SchemaSearcherTest::process_encoded_token(float_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(0 == sub_query.get_num_possible_vars());
    }
}

/**
 * Test generating vector<clp::SubQuery> for a given set of interpretations and dictionaries.
 *
 * Query: "text 100 10? 3.14*"
 * Schema Vars:
 *   int:\d+
 *   float:\d+\.\d+
 *   hasNum:[^ \$]*\d+[^ \$]*
 * Archive:
 *   Logtypes:
 *     text <int> <int> <float>
 *     text <int> <dict> <float>
 *     text <int> <dict> 3.14ab$
 *     text <int> <dict> 3.14abc$
 *     text <int> <dict> 3.15ab$
 *     text <int> 10$ <float>
 *   Vars:
 *     1a3
 *     10a
 *     10b
 * Interpretations:
 *   "text <int>(100) <int>(10?) <float)(3.14*)"
 *   "text <int>(100) <int>(10?) <c_has_num)(3.14*)"
 *   "text <int>(100) <int>(10?) 3.14*"
 *   "text <int>(100) <c_has_num>(10?) <float)(3.14*)"
 *   "text <int>(100) <c_has_num>(10?) <c_has_num)(3.14*)"
 *   "text <int>(100) <c_has_num>(10?) 3.14*"
 *   "text <int>(100) 10? <float)(3.14*)"
 *   "text <int>(100) 10? <c_has_num)(3.14*)"
 *   "text <int>(100) 10? 3.14*"
 * Subqueries will contain interpretations 0, 3, 5, 6:
 *   1+2 are omitted as the var sequence for "int, int" always ends with "float" in logtype dict
 *   4 is omitted as "int hasNum" is never followed by "hasNum" in logtype dict
 *   7+8 are omitted as static-text is always followed by "float" in the logtype dict
 */
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

    std::vector<std::vector<log_surgeon::SubQuery>> interpretations{
            {S("text "), {c_int, "100"}, S(" "), {c_int, "10?"}, S(" "), {c_float, " 3.14*"}},
            {S("text "), {c_int, "100"}, S(" "), {c_int, "10?"}, S(" "), {c_has_num, "3.14*"}},
            {S("text "), {c_int, "100"}, S(" "), {c_int, "10?"}, S(" 3.14*")},
            {S("text "), {c_int, "100"}, S(" "), {c_has_num, "10?"}, S(" "), {c_float, " 3.14*"}},
            {S("text "), {c_int, "100"}, S(" "), {c_has_num, "10?"}, S(" "), {c_has_num, "3.14*"}},
            {S("text "), {c_int, "100"}, S(" "), {c_has_num, "10?"}, S(" 3.14*")},
            {S("text "), {c_int, "100"}, S(" 10? "), {c_float, " 3.14*"}},
            {S("text "), {c_int, "100"}, S(" 10? "), {c_has_num, "3.14*"}},
            {S("text "), {c_int, "100"}, S(" 10? 3.14*")}
    };

    auto const sub_queries{clp::SchemaSearcherTest::generate_schema_sub_queries(
            interpretations,
            logtype_dict,
            var_dict
    )};

    VarInfo const wild_int{false, true, {}};
    VarInfo const wild_has_num{true, false, {1LL, 2LL}};
    size_t i{0};
    check_sub_query(i++, sub_queries, true, {wild_int, wild_has_num}, {1LL});
    check_sub_query(i++, sub_queries, true, {wild_int}, {0LL});
    check_sub_query(i++, sub_queries, true, {wild_int, wild_has_num}, {2LL, 3LL});
    check_sub_query(i++, sub_queries, true, {wild_int}, {5LL});
    REQUIRE(4 == sub_queries.size());
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

    auto txt{S("text ")};
    std::vector<std::vector<log_surgeon::SubQuery>> interpretations{
            {txt, {c_int, "100"}, S(" "), {c_int, "10?"}, S(" "), {c_float, " 3.14*"}, S("*")},
            {txt, {c_int, "100"}, S(" "), {c_int, "10?"}, S(" "), {c_has_num, "3.14*"}, S("*")},
            {txt, {c_int, "100"}, S(" "), {c_int, "10?"}, S(" 3.14**")},
            {txt, {c_int, "100"}, S(" "), {c_has_num, "10?"}, S(" "), {c_float, " 3.14*"}, S("*")},
            {txt, {c_int, "100"}, S(" "), {c_has_num, "10?"}, S(" "), {c_has_num, "3.14*"}, S("*")},
            {txt, {c_int, "100"}, S(" "), {c_has_num, "10?"}, S(" 3.14**")},
            {txt, {c_int, "100"}, S(" 10? "), {c_float, " 3.14*"}, S("*")},
            {txt, {c_int, "100"}, S(" 10? "), {c_has_num, "3.14*"}, S("*")},
            {txt, {c_int, "100"}, S(" 10? 3.14**")}
    };

    auto const sub_queries{clp::SchemaSearcherTest::generate_schema_sub_queries(
            interpretations,
            logtype_dict,
            var_dict
    )};

    VarInfo const wild_int{false, true, {}};
    VarInfo const wild_has_num{true, true, {1LL}};
    size_t i{0};
    check_sub_query(i++, sub_queries, true, {wild_int, wild_has_num}, {1LL});
    check_sub_query(i++, sub_queries, true, {wild_int}, {0LL});
    check_sub_query(i++, sub_queries, true, {wild_int, wild_has_num}, {2LL, 3LL});
    check_sub_query(i++, sub_queries, true, {wild_int}, {5LL});
    REQUIRE(4 == sub_queries.size());
}
