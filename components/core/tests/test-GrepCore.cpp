#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <log_surgeon/Constants.hpp>
#include <log_surgeon/Lexer.hpp>
#include <log_surgeon/Schema.hpp>
#include <log_surgeon/SchemaParser.hpp>
#include <log_surgeon/wildcard_query_parser/QueryInterpretation.hpp>

#include "../src/clp/Defs.h"
#include "../src/clp/EncodedVariableInterpreter.hpp"
#include "../src/clp/GrepCore.hpp"
#include "../src/clp/LogTypeDictionaryReaderReq.hpp"
#include "../src/clp/Query.hpp"
#include "../src/clp/string_utils/string_utils.hpp"
#include "../src/clp/VariableDictionaryReaderReq.hpp"

using clp::EncodedVariableInterpreter;
using clp::GrepCore;
using clp::LogTypeDictionaryReaderReq;
using clp::string_utils::wildcard_match_unsafe_case_sensitive;
using clp::SubQuery;
using clp::variable_dictionary_id_t;
using clp::VariableDictionaryReaderReq;
using log_surgeon::lexers::ByteLexer;
using log_surgeon::Schema;
using log_surgeon::SchemaVarAST;
using log_surgeon::SymbolId::TokenFloat;
using log_surgeon::SymbolId::TokenInt;
using log_surgeon::wildcard_query_parser::QueryInterpretation;
using log_surgeon::wildcard_query_parser::VariableQueryToken;
using std::pair;
using std::set;
using std::string;
using std::string_view;
using std::tuple;
using std::unordered_map;
using std::unordered_set;
using std::variant;
using std::vector;

constexpr uint32_t cIntId{static_cast<uint32_t>(TokenInt)};
constexpr uint32_t cFloatId{static_cast<uint32_t>(TokenFloat)};
constexpr uint32_t cHasNumId{111};

/**
 * Helper to expose `GrepCore` functionality for unit-testing.
 *
 * This class provides static wrappers around `GrepCore` methods, allowing test
 * code to access internal logic such as:
 * - Finding wildcard encodable positions in a `QueryInterpretation`;
 * - Generating logtype strings with wildcard masks;
 * - Processing variable tokens with or without encoding;
 * - Generating schema-based sub-queries.
 *
 * All methods forward directly to `GrepCore` and are intended for testing only.
 */
class clp::GrepCoreTest {
public:
    static auto normalize_interpretations(set<QueryInterpretation> const& interpretations)
            -> set<QueryInterpretation> {
        return GrepCore::normalize_interpretations(interpretations);
    }

    template <
            LogTypeDictionaryReaderReq LogTypeDictionaryReaderType,
            VariableDictionaryReaderReq VariableDictionaryReaderType
    >
    static auto generate_schema_sub_queries(
            std::set<QueryInterpretation> const& interpretations,
            LogTypeDictionaryReaderType const& logtype_dict,
            VariableDictionaryReaderType const& var_dict,
            std::vector<SubQuery>& sub_queries
    ) -> void {
        GrepCore::generate_schema_sub_queries(
                interpretations,
                logtype_dict,
                var_dict,
                false,
                sub_queries
        );
    }

    static auto get_wildcard_encodable_positions(QueryInterpretation const& interpretation)
            -> vector<size_t> {
        return GrepCore::get_wildcard_encodable_positions(interpretation);
    }

    static auto generate_logtype_string(
            QueryInterpretation const& interpretation,
            unordered_map<size_t, bool> const& wildcard_mask_map
    ) -> string {
        return GrepCore::generate_logtype_string(interpretation, wildcard_mask_map);
    }

    template <typename VariableDictionaryReaderType>
    static auto process_token(
            VariableQueryToken const& var_token,
            VariableDictionaryReaderType const& var_dict,
            SubQuery& sub_query
    ) -> bool {
        return GrepCore::process_schema_var_token(var_token, var_dict, false, false, sub_query);
    }

    template <typename VariableDictionaryReaderType>
    static auto process_encoded_token(
            VariableQueryToken const& var_token,
            VariableDictionaryReaderType const& var_dict,
            SubQuery& sub_query
    ) -> bool {
        return GrepCore::process_schema_var_token(var_token, var_dict, false, true, sub_query);
    }
};

namespace {
/**
 * Simple helper class representing a fake variable dictionary entry for unit tests.
 *
 * Adheres to `VariableDictionaryEntryReq`.
 */
class FakeVarEntry {
public:
    explicit FakeVarEntry(variable_dictionary_id_t const id, string value)
            : m_id{id},
              m_value{std::move(value)} {}

    [[nodiscard]] auto get_id() const -> variable_dictionary_id_t { return m_id; }

    [[nodiscard]] auto get_value() const -> string const& { return m_value; }

private:
    variable_dictionary_id_t m_id;
    string m_value;
};

/**
 * Simple helper class representing a fake variable dictionary for unit tests.
 *
 * Provides a method for adding entries and adheres to `VariableDictionaryReaderReq`.
 */
class FakeVarDict {
public:
    using Entry = FakeVarEntry;
    using dictionary_id_t = variable_dictionary_id_t;

    auto add_entry(dictionary_id_t const id, string value) -> void {
        m_storage.emplace(id, Entry{id, std::move(value)});
    }

    [[nodiscard]] auto get_value(dictionary_id_t const id) const -> string const& {
        static string const cEmpty{};
        if (m_storage.contains(id)) {
            return m_storage.at(id).get_value();
        }
        return cEmpty;
    }

    auto get_entry_matching_value(string_view const val, [[maybe_unused]] bool ignore_case) const
            -> vector<Entry const*> {
        vector<Entry const*> results;
        for (auto const& [id, entry] : m_storage) {
            if (val == entry.get_value()) {
                results.push_back(&entry);
            }
        }
        return results;
    }

    auto get_entries_matching_wildcard_string(
            string_view const val,
            [[maybe_unused]] bool ignore_case,
            unordered_set<Entry const*>& results
    ) const -> void {
        for (auto const& [id, entry] : m_storage) {
            if (wildcard_match_unsafe_case_sensitive(entry.get_value(), val)) {
                results.insert(&entry);
            }
        }
    }

private:
    unordered_map<dictionary_id_t, Entry> m_storage;
};

/**
 * Simple helper class representing a fake logtype dictionary entry for unit tests.
 *
 * Adheres to `LogtypeDictionaryEntryReq`.
 */
class FakeLogTypeEntry {
public:
    FakeLogTypeEntry(string value, clp::logtype_dictionary_id_t const id)
            : m_value(std::move(value)),
              m_id(id) {}

    auto clear() -> void { m_value.clear(); }

    auto reserve_constant_length(size_t const length) -> void { m_value.reserve(length); }

    auto parse_next_var(
            [[maybe_unused]] string_view msg,
            [[maybe_unused]] size_t begin,
            [[maybe_unused]] size_t end,
            [[maybe_unused]] string_view& parsed
    ) -> bool {
        return false;
    }

    auto add_constant(string_view const msg, size_t const begin_pos, size_t const length) -> void {
        m_value.append(msg.substr(begin_pos, length));
    }

    auto add_int_var() -> void { EncodedVariableInterpreter::add_int_var(m_value); }

    auto add_float_var() -> void { EncodedVariableInterpreter::add_float_var(m_value); }

    auto add_dictionary_var() -> void { EncodedVariableInterpreter::add_dict_var(m_value); }

    [[nodiscard]] auto get_value() const -> string const& { return m_value; }

    [[nodiscard]] auto get_num_variables() const -> size_t { return 0; }

    [[nodiscard]] auto get_num_placeholders() const -> size_t { return 0; }

    [[nodiscard]] auto
    get_placeholder_info([[maybe_unused]] size_t idx, [[maybe_unused]] auto& ref) const -> size_t {
        return SIZE_MAX;
    }

    [[nodiscard]] auto get_id() const -> clp::logtype_dictionary_id_t { return m_id; }

private:
    string m_value;
    clp::logtype_dictionary_id_t m_id{0};
};

/**
 * Simple helper class representing a fake logtype dictionary for unit tests.
 *
 * Provides a method for adding entries and adheres to `LogtypeDictionaryReaderReq`.
 */
class FakeLogTypeDict {
public:
    using Entry = FakeLogTypeEntry;
    using dictionary_id_t = clp::logtype_dictionary_id_t;

    auto add_entry(string const& value, dictionary_id_t id) -> void {
        m_storage.emplace_back(value, id);
    }

    auto
    get_entry_matching_value(string_view const logtype, [[maybe_unused]] bool ignore_case) const
            -> vector<Entry const*> {
        vector<Entry const*> results;
        for (auto const& entry : m_storage) {
            if (logtype == entry.get_value()) {
                results.push_back(&entry);
            }
        }
        return results;
    }

    auto get_entries_matching_wildcard_string(
            string_view const logtype,
            [[maybe_unused]] bool ignore_case,
            unordered_set<Entry const*>& results
    ) const -> void {
        for (auto const& entry : m_storage) {
            if (wildcard_match_unsafe_case_sensitive(entry.get_value(), logtype)) {
                results.insert(&entry);
            }
        }
    }

private:
    vector<Entry> m_storage;
};

/**
 * @param entries Vector of (id, value) pairs to populate the variable
 * dictionary.
 * @return A `FakeVarDict` initialized with the given entries.
 */
auto make_var_dict(vector<pair<size_t, string>> const& entries) -> FakeVarDict;

/**
 * @param entries Vector of logtypes, where each logtype is represented by a vector of tokens. Each
 * token is either a literal substring (`string_view`) or a variable placeholder (`char`).
 * @return A `FakeLogtypeDict` initialized with the given entries.
 */
auto make_logtype_dict(vector<vector<variant<string_view, char>>> const& entries)
        -> FakeLogTypeDict;

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

/**
 * Generates a logtype string from a vector of tokens.
 *
 * Each token is either:
 * - a literal substring (`string_view`) to append directly, or
 * - a variable placeholder (`char`) indicating the type of variable:
 *   - `i` -> integer variable;
 *   - `f` -> float variable;
 *   - `d` -> dictionary variable.
 *
 * The function forwards variable tokens to `EncodedVariableInterpreter` to
 * append their encoded representations to the resulting string.
 *
 * @param tokens Vector of tokens to convert into a logtype string.
 * @return A `string` representing the expected encoded logtype.
 */
auto generate_expected_logtype_string(vector<variant<string_view, char>> const& tokens) -> string;

/**
 * Checks that a `SubQuery` at a given index matches the expected properties.
 *
 * This method verifies:
 * - Whether wildcard matching is required;
 * - The number and type of variables;
 * - For dictionary variables, the precise or possible dictionary IDs;
 * - The set of possible logtype IDs.
 *
 * @param id Index of the sub-query to check in `sub_queries`.
 * @param sub_queries Vector of `SubQuery` objects.
 * @param wildcard_match_required Expected wildcard match requirement.
 * @param vars_info Vector of tuples describing expected variable properties: (`is_dict_var`,
 * `is_precise_var`, `var_dict_ids`).
 * @param logtype_ids Expected set of possible logtype IDs.
 */
auto check_sub_query(
        size_t id,
        vector<SubQuery> const& sub_queries,
        bool wildcard_match_required,
        vector<tuple<bool, bool, unordered_set<size_t>>> const& vars_info,
        unordered_set<clp::logtype_dictionary_id_t> const& logtype_ids
) -> void;

/**
 * Initializes a `ByteLexer` with space as a delimiter and the given `schema_rules`.
 *
 * @param schema_rules A vector of strings, each string representing a schema rule.
 * @return The initialized `ByteLexer`.
 */
auto make_test_lexer(vector<string> const& schema_rules) -> ByteLexer;

auto make_var_dict(vector<pair<size_t, string>> const& entries) -> FakeVarDict {
    FakeVarDict dict;
    for (auto const& [id, val] : entries) {
        dict.add_entry(id, val);
    }
    return dict;
}

auto make_logtype_dict(vector<vector<variant<string_view, char>>> const& entries)
        -> FakeLogTypeDict {
    FakeLogTypeDict dict;
    clp::logtype_dictionary_id_t id{0};
    for (auto const& entry : entries) {
        dict.add_entry(generate_expected_logtype_string(entry), id++);
    }
    return dict;
}

auto make_query_interpretation(vector<variant<string, pair<uint32_t, string>>> const& tokens)
        -> QueryInterpretation {
    QueryInterpretation interp;
    for (auto const& token : tokens) {
        if (holds_alternative<string>(token)) {
            interp.append_static_token(get<string>(token));
        } else {
            auto const& [symbol, value]{get<pair<uint32_t, string>>(token)};
            auto const contains_wildcard{value.find_first_of("*?") != string::npos};
            interp.append_variable_token(symbol, value, contains_wildcard);
        }
    }
    return interp;
}

auto generate_expected_logtype_string(vector<variant<string_view, char>> const& tokens) -> string {
    string result;
    for (auto const& token : tokens) {
        if (holds_alternative<string_view>(token)) {
            result.append(get<string_view>(token));
        } else {
            switch (get<char>(token)) {
                case 'i':
                    EncodedVariableInterpreter::add_int_var(result);
                    break;
                case 'f':
                    EncodedVariableInterpreter::add_float_var(result);
                    break;
                case 'd':
                    EncodedVariableInterpreter::add_dict_var(result);
                    break;
                default:
                    break;
            }
        }
    }
    return result;
}

auto check_sub_query(
        size_t id,
        vector<SubQuery> const& sub_queries,
        bool const wildcard_match_required,
        vector<tuple<bool, bool, unordered_set<size_t>>> const& vars_info,
        unordered_set<clp::logtype_dictionary_id_t> const& logtype_ids
) -> void {
    CAPTURE(id);
    auto const& sub_query{sub_queries[id]};

    REQUIRE(wildcard_match_required == sub_query.wildcard_match_required());
    REQUIRE(vars_info.size() == sub_query.get_num_possible_vars());

    for (size_t i{0}; i < vars_info.size(); ++i) {
        auto const& [is_dict_var, is_precise_var, var_dict_ids]{vars_info[i]};
        auto const& var{sub_query.get_vars()[i]};
        REQUIRE(is_dict_var == var.is_dict_var());
        REQUIRE(is_precise_var == var.is_precise_var());
        if (is_dict_var) {
            if (is_precise_var) {
                REQUIRE(1 == var_dict_ids.size());
                REQUIRE(var_dict_ids.contains(var.get_var_dict_id()));
            } else {
                REQUIRE(var_dict_ids == var.get_possible_var_dict_ids());
            }
        }
    }

    REQUIRE(logtype_ids == sub_query.get_possible_logtypes());
}

auto make_test_lexer(vector<string> const& schema_rules) -> ByteLexer {
    ByteLexer lexer;
    lexer.m_symbol_id["int"] = cIntId;
    lexer.m_symbol_id["float"] = cFloatId;
    lexer.m_symbol_id["hasNumber"] = cHasNumId;
    lexer.m_id_symbol[cIntId] = "int";
    lexer.m_id_symbol[cFloatId] = "float";
    lexer.m_id_symbol[cHasNumId] = "hasNumber";
    lexer.set_delimiters({' '});

    Schema schema;
    for (auto const& schema_rule : schema_rules) {
        schema.add_variable(schema_rule, -1);
    }

    auto const schema_ast = schema.release_schema_ast_ptr();
    REQUIRE(nullptr != schema_ast);
    REQUIRE(schema_rules.size() == schema_ast->m_schema_vars.size());
    for (size_t i{0}; i < schema_ast->m_schema_vars.size(); ++i) {
        REQUIRE(nullptr != schema_ast->m_schema_vars[i]);
        auto* capture_rule_ast{dynamic_cast<SchemaVarAST*>(schema_ast->m_schema_vars[i].get())};
        REQUIRE(nullptr != capture_rule_ast);
        lexer.add_rule(
                lexer.m_symbol_id[capture_rule_ast->m_name],
                std::move(capture_rule_ast->m_regex_ptr)
        );
    }

    lexer.generate();
    return lexer;
}
}  //  namespace

// Tests: `get_wildcard_encodable_positions`
TEST_CASE("get_wildcard_encodable_positions_for_empty_interpretation", "[dfa_search]") {
    QueryInterpretation const interpretation{};

    auto const positions{clp::GrepCoreTest::get_wildcard_encodable_positions(interpretation)};
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

    auto const positions{clp::GrepCoreTest::get_wildcard_encodable_positions(interpretation)};
    REQUIRE(2 == positions.size());
    REQUIRE(3 == positions[0]);
    REQUIRE(4 == positions[1]);
}

// Tests: `generate_logtype_string`
TEST_CASE("generate_logtype_string_for_empty_interpretation", "[dfa_search]") {
    QueryInterpretation const interpretation{};

    auto const wildcard_encodable_positions{
            clp::GrepCoreTest::get_wildcard_encodable_positions(interpretation)
    };
    uint64_t const num_combos{1ULL << wildcard_encodable_positions.size()};
    REQUIRE(1 == num_combos);
    for (uint64_t mask{0}; mask < num_combos; ++mask) {
        std::unordered_map<size_t, bool> wildcard_mask_map;
        for (size_t i{0}; i < wildcard_encodable_positions.size(); ++i) {
            wildcard_mask_map[wildcard_encodable_positions[i]] = (mask >> i) & 1ULL;
        }
        auto logtype_string{
                clp::GrepCoreTest::generate_logtype_string(interpretation, wildcard_mask_map)
        };
        REQUIRE(logtype_string.empty());
    }
}

TEST_CASE("generate_logtype_string_for_single_variable_interpretation", "[dfa_search]") {
    string expected_logtype_string;
    EncodedVariableInterpreter::add_int_var(expected_logtype_string);

    QueryInterpretation interpretation{};
    interpretation.append_variable_token(static_cast<uint32_t>(TokenInt), "100", false);

    auto const wildcard_encodable_positions{
            clp::GrepCoreTest::get_wildcard_encodable_positions(interpretation)
    };
    uint64_t const num_combos{1ULL << wildcard_encodable_positions.size()};
    REQUIRE(1 == num_combos);

    std::unordered_map<size_t, bool> const wildcard_mask_map{};
    auto logtype_string{
            clp::GrepCoreTest::generate_logtype_string(interpretation, wildcard_mask_map)
    };
    REQUIRE(expected_logtype_string == logtype_string);
}

TEST_CASE("generate_logtype_string_for_multi_variable_interpretation", "[dfa_search]") {
    unordered_set<string> const expected_logtype_strings{
            {{generate_expected_logtype_string({"text", 'i', 'f', 'd', 'd', 'd'})},
             {generate_expected_logtype_string({"text", 'i', 'f', 'i', 'd', 'd'})},
             {generate_expected_logtype_string({"text", 'i', 'f', 'd', 'f', 'd'})},
             {generate_expected_logtype_string({"text", 'i', 'f', 'i', 'f', 'd'})}}
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
            clp::GrepCoreTest::get_wildcard_encodable_positions(interpretation)
    };

    uint64_t const num_combos{1ULL << wildcard_encodable_positions.size()};
    REQUIRE(num_combos == 4);
    unordered_set<string> logtype_strings;
    for (uint64_t mask{0}; mask < num_combos; ++mask) {
        unordered_map<size_t, bool> wildcard_mask_map;
        for (size_t i{0}; i < wildcard_encodable_positions.size(); ++i) {
            wildcard_mask_map[wildcard_encodable_positions[i]] = (mask >> i) & 1ULL;
        }
        logtype_strings.insert(
                clp::GrepCoreTest::generate_logtype_string(interpretation, wildcard_mask_map)
        );
    }
    REQUIRE(expected_logtype_strings == logtype_strings);
}

// Tests: `process_schema_var_token`
TEST_CASE("process_schema_empty_token ", "[dfa_search]") {
    FakeVarDict const var_dict{make_var_dict({pair{0, "100"}})};

    SubQuery sub_query;
    VariableQueryToken const empty_int_token{cIntId, "", false};
    REQUIRE(false == clp::GrepCoreTest::process_token(empty_int_token, var_dict, sub_query));
    REQUIRE(false == sub_query.wildcard_match_required());
    REQUIRE(0 == sub_query.get_num_possible_vars());
}

TEST_CASE("process_schema_unmatched_token ", "[dfa_search]") {
    FakeVarDict const var_dict{make_var_dict({pair{0, "100"}})};

    SubQuery sub_query;
    VariableQueryToken const int_token{cIntId, "200", false};
    REQUIRE(clp::GrepCoreTest::process_token(int_token, var_dict, sub_query));
    REQUIRE(false == sub_query.wildcard_match_required());
    REQUIRE(1 == sub_query.get_num_possible_vars());
    auto const& var{sub_query.get_vars()[0]};
    REQUIRE(false == var.is_dict_var());
    REQUIRE(var.is_precise_var());
    REQUIRE(var.get_possible_var_dict_ids().empty());
}

TEST_CASE("process_schema_int_token ", "[dfa_search]") {
    FakeVarDict const var_dict{make_var_dict({pair{0, "100"}})};

    SubQuery sub_query;
    VariableQueryToken const int_token{cIntId, "100", false};
    REQUIRE(clp::GrepCoreTest::process_token(int_token, var_dict, sub_query));
    REQUIRE(false == sub_query.wildcard_match_required());
    REQUIRE(1 == sub_query.get_num_possible_vars());
    auto const& var{sub_query.get_vars()[0]};
    REQUIRE(false == var.is_dict_var());
    REQUIRE(var.is_precise_var());
    REQUIRE(var.get_possible_var_dict_ids().empty());
}

TEST_CASE("process_schema_encoded_non_greedy_wildcard_token ", "[dfa_search]") {
    FakeVarDict const var_dict{make_var_dict({pair{0, "10a0"}, pair{1, "10b0"}})};

    SECTION("interpret_as_int") {
        SubQuery sub_query;
        VariableQueryToken const int_token{cIntId, "10?0", true};
        REQUIRE(clp::GrepCoreTest::process_encoded_token(int_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(0 == sub_query.get_num_possible_vars());
    }

    SECTION("interpret_as_float") {
        SubQuery sub_query;
        VariableQueryToken const float_token{cFloatId, "10?0", true};
        REQUIRE(clp::GrepCoreTest::process_encoded_token(float_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(0 == sub_query.get_num_possible_vars());
    }

    SECTION("interpret_as_precise_has_number") {
        SubQuery sub_query;
        VariableQueryToken const has_number_token{cHasNumId, "10a?", true};
        REQUIRE(clp::GrepCoreTest::process_token(has_number_token, var_dict, sub_query));
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
        REQUIRE(clp::GrepCoreTest::process_token(has_number_token, var_dict, sub_query));
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
TEST_CASE("process_schema_non_encoded_non_greedy_wildcard_token ", "[dfa_search]") {
    size_t id{0};
    FakeVarDict const var_dict{make_var_dict(
            {pair{id++, "100000000000000000000000010"},
             pair{id++, "100000000000000000000000020"},
             pair{id++, "100000000000000000000000030"},
             pair{id++, "1000000000000000000000000.0"},
             pair{id++, "1000000000000000000000000a0"}}
    )};

    SECTION("interpret_as_int") {
        SubQuery sub_query;
        VariableQueryToken const int_token{cIntId, "1000000000000000000000000?0", true};
        REQUIRE(clp::GrepCoreTest::process_token(int_token, var_dict, sub_query));
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
        REQUIRE(clp::GrepCoreTest::process_token(float_token, var_dict, sub_query));
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
        REQUIRE(clp::GrepCoreTest::process_token(has_number_token, var_dict, sub_query));
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

TEST_CASE("process_schema_greedy_wildcard_token ", "[dfa_search]") {
    size_t id{0};
    FakeVarDict const var_dict{make_var_dict(
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
        REQUIRE(clp::GrepCoreTest::process_token(int_token, var_dict, sub_query));
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
        REQUIRE(clp::GrepCoreTest::process_token(float_token, var_dict, sub_query));
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
        REQUIRE(clp::GrepCoreTest::process_token(has_number_token, var_dict, sub_query));
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
        REQUIRE(clp::GrepCoreTest::process_token(has_number_token, var_dict, sub_query));
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
        REQUIRE(clp::GrepCoreTest::process_encoded_token(int_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(0 == sub_query.get_num_possible_vars());
    }

    SECTION("interpret_as_encoded_float") {
        SubQuery sub_query;
        VariableQueryToken const float_token{cFloatId, "10*0", true};
        REQUIRE(clp::GrepCoreTest::process_encoded_token(float_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(0 == sub_query.get_num_possible_vars());
    }
}

// Tests: `generate_schema_sub_queries`
TEST_CASE("generate_schema_sub_queries", "[dfa_search]") {
    FakeVarDict const var_dict{make_var_dict({pair{0, "10a"}, pair{1, "1a3"}})};
    FakeLogTypeDict const logtype_dict{make_logtype_dict(
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

    vector<SubQuery> sub_queries;
    clp::GrepCoreTest::generate_schema_sub_queries(
            interpretations,
            logtype_dict,
            var_dict,
            sub_queries
    );

    using Var = tuple<bool, bool, unordered_set<size_t>>;
    REQUIRE(6 == sub_queries.size());
    size_t i{0};
    // NOTE: sub queries 0 and 2 are a duplicate of 3 and 5 because we use a vector instead of a set
    // when storing `m_sub_queries` in `Query`.
    check_sub_query(i++, sub_queries, true, {Var{false, true, {}}, Var{true, true, {0}}}, {1});
    check_sub_query(i++, sub_queries, true, {Var{false, true, {}}}, {0});
    check_sub_query(i++, sub_queries, false, {Var{false, true, {}}, Var{true, true, {0}}}, {2, 3});
    check_sub_query(i++, sub_queries, true, {Var{false, true, {}}, Var{true, true, {0}}}, {1});
    check_sub_query(i++, sub_queries, false, {Var{false, true, {}}, Var{true, true, {0}}}, {2, 3});
    check_sub_query(i++, sub_queries, true, {Var{false, true, {}}}, {5});
}

TEST_CASE("generate_schema_sub_queries_with_wildcard_duplication", "[dfa_search]") {
    FakeVarDict const var_dict{make_var_dict({pair{0, "10a"}, pair{1, "1a3"}})};
    FakeLogTypeDict const logtype_dict{make_logtype_dict(
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
            clp::GrepCoreTest::normalize_interpretations(interpretations)
    };

    vector<SubQuery> sub_queries;
    clp::GrepCoreTest::generate_schema_sub_queries(
            normalized_interpretations,
            logtype_dict,
            var_dict,
            sub_queries
    );

    using Var = tuple<bool, bool, unordered_set<size_t>>;
    REQUIRE(6 == sub_queries.size());
    size_t i{0};
    check_sub_query(i++, sub_queries, true, {Var{false, true, {}}, Var{true, true, {0}}}, {1});
    check_sub_query(i++, sub_queries, true, {Var{false, true, {}}}, {0});
    check_sub_query(i++, sub_queries, false, {Var{false, true, {}}, Var{true, true, {0}}}, {2, 3});
    check_sub_query(i++, sub_queries, true, {Var{false, true, {}}, Var{true, true, {0}}}, {1});
    check_sub_query(i++, sub_queries, false, {Var{false, true, {}}, Var{true, true, {0}}}, {2, 3});
    check_sub_query(i++, sub_queries, true, {Var{false, true, {}}}, {5});
}

// Tests: `process_raw_query`
TEST_CASE("process_raw_query", "[dfa_search]") {
    auto lexer{make_test_lexer(
            {{R"(int:(\d+))"}, {R"(float:(\d+\.\d+))"}, {R"(hasNumber:[^ $]*\d+[^ $]*)"}}
    )};

    FakeVarDict const var_dict{make_var_dict({pair{0, "10a"}, pair{1, "1a3"}})};
    FakeLogTypeDict const logtype_dict{make_logtype_dict(
            {{"text ", 'i', " ", 'i', " ", 'f'},
             {"text ", 'i', " ", 'd', " ", 'f'},
             {"text ", 'i', " ", 'd', " 3.14ab$"},
             {"text ", 'i', " ", 'd', " 3.14abc$"},
             {"text ", 'i', " ", 'd', " 3.15ab$"},
             {"text ", 'i', " 10$ ", 'f'}}
    )};

    string const raw_query{"text 100 10? 3.14*"};

    auto const query{
            GrepCore::process_raw_query(logtype_dict, var_dict, raw_query, 0, 0, true, lexer, false)
    };

    REQUIRE(query.has_value());

    using Var = tuple<bool, bool, unordered_set<size_t>>;
    auto const& sub_queries{query.value().get_sub_queries()};
    REQUIRE(6 == sub_queries.size());
    size_t i{0};
    check_sub_query(i++, sub_queries, true, {Var{false, true, {}}, Var{true, true, {0}}}, {1});
    check_sub_query(i++, sub_queries, true, {Var{false, true, {}}}, {0});
    check_sub_query(i++, sub_queries, false, {Var{false, true, {}}, Var{true, true, {0}}}, {2, 3});
    check_sub_query(i++, sub_queries, true, {Var{false, true, {}}, Var{true, true, {0}}}, {1});
    check_sub_query(i++, sub_queries, false, {Var{false, true, {}}, Var{true, true, {0}}}, {2, 3});
    check_sub_query(i++, sub_queries, true, {Var{false, true, {}}}, {5});
}

// Tests: `get_bounds_of_next_potential_var`
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
