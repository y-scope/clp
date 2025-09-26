#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <log_surgeon/Constants.hpp>
#include <log_surgeon/Lexer.hpp>
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
using log_surgeon::SymbolId::TokenFloat;
using log_surgeon::SymbolId::TokenInt;
using log_surgeon::wildcard_query_parser::QueryInterpretation;
using log_surgeon::wildcard_query_parser::VariableQueryToken;
using std::make_unique;
using std::set;
using std::string;
using std::string_view;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::vector;

class clp::GrepCoreTest {
public:
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

    template <
            LogTypeDictionaryReaderReq LogTypeDictionaryReaderType,
            VariableDictionaryReaderReq VariableDictionaryReaderType>
    static void generate_schema_sub_queries(
            std::set<QueryInterpretation> const& interpretations,
            LogTypeDictionaryReaderType const& logtype_dict,
            VariableDictionaryReaderType const& var_dict,
            std::vector<SubQuery>& sub_queries
    ) {
        GrepCore::generate_schema_sub_queries(
                interpretations,
                logtype_dict,
                var_dict,
                false,
                sub_queries
        );
    }
};

namespace {
class FakeVarEntry {
public:
    explicit FakeVarEntry(variable_dictionary_id_t const id, string value)
            : m_id{id},
              m_value{value} {}

    [[nodiscard]] auto get_id() const -> variable_dictionary_id_t { return m_id; }

    [[nodiscard]] auto get_value() const -> string const& { return m_value; }

private:
    variable_dictionary_id_t m_id;
    string m_value;
};

class FakeVarDict {
public:
    using Entry = FakeVarEntry;
    using dictionary_id_t = variable_dictionary_id_t;

    auto add_entry(dictionary_id_t const id, string value) -> void {
        m_storage.emplace(id, Entry{id, std::move(value)});
    }

    [[nodiscard]] auto get_value(dictionary_id_t const id) const -> string const& {
        static string const empty{};
        if (m_storage.contains(id)) {
            return m_storage.at(id).get_value();
        }
        return empty;
    }

    auto get_entry_matching_value(string_view const val, bool ignore_case) const
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
            bool ignore_case,
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

class FakeLogTypeEntry {
public:
    FakeLogTypeEntry(string const value, clp::logtype_dictionary_id_t const id)
            : m_value(value),
              m_id(id) {}

    auto clear() -> void { m_value.clear(); }

    auto reserve_constant_length(size_t length) -> void { m_value.reserve(length); }

    auto parse_next_var(string_view msg, size_t begin, size_t end, string_view& parsed) -> bool {
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

    [[nodiscard]] auto get_placeholder_info(size_t idx, auto& ref) const -> size_t {
        return SIZE_MAX;
    }

    [[nodiscard]] auto get_id() const -> clp::logtype_dictionary_id_t { return m_id; }

private:
    string m_value;
    clp::logtype_dictionary_id_t m_id{0};
};

class FakeLogTypeDict {
public:
    using Entry = FakeLogTypeEntry;
    using dictionary_id_t = clp::logtype_dictionary_id_t;

    auto add_entry(string const& value, dictionary_id_t id) -> void {
        m_storage.emplace_back(value, id);
    }

    auto get_entry_matching_value(string_view const logtype, bool ignore_case) const
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
            bool ignore_case,
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
}  //  namespace

// Tests: `get_wildcard_encodable_positions`
TEST_CASE("get_wildcard_encodable_positions_for_empty_interpretation", "[dfa_search]") {
    QueryInterpretation const interpretation{};

    auto const positions{clp::GrepCoreTest::get_wildcard_encodable_positions(interpretation)};
    REQUIRE(positions.empty());
}

TEST_CASE("get_wildcard_encodable_positions_for_multi_variable_interpretation", "[dfa_search]") {
    constexpr uint32_t cHasNumberId{100};

    QueryInterpretation interpretation{};
    interpretation.append_static_token("static_text");
    interpretation.append_variable_token(static_cast<uint32_t>(TokenInt), "100", false);
    interpretation.append_variable_token(static_cast<uint32_t>(TokenFloat), "32.2", false);
    interpretation.append_variable_token(static_cast<uint32_t>(TokenInt), "10?", true);
    interpretation.append_variable_token(static_cast<uint32_t>(TokenFloat), "3.14*", true);
    interpretation.append_variable_token(cHasNumberId, "3.14*", true);

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
            wildcard_mask_map[wildcard_encodable_positions[i]] = mask >> i & 1ULL;
        }
        auto logtype_string{
                clp::GrepCoreTest::generate_logtype_string(interpretation, wildcard_mask_map)
        };
        REQUIRE("" == logtype_string);
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
    size_t const num_combos{static_cast<size_t>(1) << wildcard_encodable_positions.size()};
    REQUIRE(1 == num_combos);

    std::unordered_map<size_t, bool> const wildcard_mask_map{false};
    auto logtype_string{
            clp::GrepCoreTest::generate_logtype_string(interpretation, wildcard_mask_map)
    };
    REQUIRE(expected_logtype_string == logtype_string);
}

TEST_CASE("generate_logtype_string_for_multi_variable_interpretation", "[dfa_search]") {
    constexpr uint32_t cHasNumberId{100};

    vector<string> expected_logtype_strings;
    expected_logtype_strings.push_back("static_text");
    EncodedVariableInterpreter::add_int_var(expected_logtype_strings.back());
    EncodedVariableInterpreter::add_float_var(expected_logtype_strings.back());
    EncodedVariableInterpreter::add_dict_var(expected_logtype_strings.back());
    EncodedVariableInterpreter::add_dict_var(expected_logtype_strings.back());
    EncodedVariableInterpreter::add_dict_var(expected_logtype_strings.back());

    expected_logtype_strings.push_back("static_text");
    EncodedVariableInterpreter::add_int_var(expected_logtype_strings.back());
    EncodedVariableInterpreter::add_float_var(expected_logtype_strings.back());
    EncodedVariableInterpreter::add_int_var(expected_logtype_strings.back());
    EncodedVariableInterpreter::add_dict_var(expected_logtype_strings.back());
    EncodedVariableInterpreter::add_dict_var(expected_logtype_strings.back());

    expected_logtype_strings.push_back("static_text");
    EncodedVariableInterpreter::add_int_var(expected_logtype_strings.back());
    EncodedVariableInterpreter::add_float_var(expected_logtype_strings.back());
    EncodedVariableInterpreter::add_dict_var(expected_logtype_strings.back());
    EncodedVariableInterpreter::add_float_var(expected_logtype_strings.back());
    EncodedVariableInterpreter::add_dict_var(expected_logtype_strings.back());

    expected_logtype_strings.push_back("static_text");
    EncodedVariableInterpreter::add_int_var(expected_logtype_strings.back());
    EncodedVariableInterpreter::add_float_var(expected_logtype_strings.back());
    EncodedVariableInterpreter::add_int_var(expected_logtype_strings.back());
    EncodedVariableInterpreter::add_float_var(expected_logtype_strings.back());
    EncodedVariableInterpreter::add_dict_var(expected_logtype_strings.back());

    QueryInterpretation interpretation{};
    interpretation.append_static_token("static_text");
    interpretation.append_variable_token(static_cast<uint32_t>(TokenInt), "100", false);
    interpretation.append_variable_token(static_cast<uint32_t>(TokenFloat), "32.2", false);
    interpretation.append_variable_token(static_cast<uint32_t>(TokenInt), "10?", true);
    interpretation.append_variable_token(static_cast<uint32_t>(TokenFloat), "3.14*", true);
    interpretation.append_variable_token(cHasNumberId, "3.14*", true);

    auto const wildcard_encodable_positions{
            clp::GrepCoreTest::get_wildcard_encodable_positions(interpretation)
    };

    size_t const num_combos{static_cast<size_t>(1) << wildcard_encodable_positions.size()};
    REQUIRE(num_combos == 4);
    for (size_t mask{0}; mask < num_combos; ++mask) {
        unordered_map<size_t, bool> wildcard_mask_map;
        for (size_t i{0}; i < wildcard_encodable_positions.size(); ++i) {
            wildcard_mask_map[wildcard_encodable_positions[i]] = mask >> i & 1;
        }
        auto logtype_string{
                clp::GrepCoreTest::generate_logtype_string(interpretation, wildcard_mask_map)
        };
        CAPTURE(mask);
        REQUIRE(expected_logtype_strings[mask] == logtype_string);
    }
}

// Tests: `process_schema_var_token`
TEST_CASE("process_schema_empty_token ", "[dfa_search]") {
    FakeVarDict var_dict;
    var_dict.add_entry(0, "100");

    SubQuery sub_query;
    VariableQueryToken const static_token{0, "", false};
    REQUIRE(false == clp::GrepCoreTest::process_token(static_token, var_dict, sub_query));
    REQUIRE(false == sub_query.wildcard_match_required());
    REQUIRE(0 == sub_query.get_num_possible_vars());
}

TEST_CASE("process_schema_unmatched_token ", "[dfa_search]") {
    FakeVarDict var_dict;
    var_dict.add_entry(0, "100");

    SubQuery sub_query;
    VariableQueryToken const static_token{0, "200", false};
    REQUIRE(false == clp::GrepCoreTest::process_token(static_token, var_dict, sub_query));
    REQUIRE(false == sub_query.wildcard_match_required());
    REQUIRE(0 == sub_query.get_num_possible_vars());
}

TEST_CASE("process_schema_int_token ", "[dfa_search]") {
    FakeVarDict var_dict;
    var_dict.add_entry(0, "100");

    SubQuery sub_query;
    VariableQueryToken const int_token{0, "100", false};
    REQUIRE(clp::GrepCoreTest::process_token(int_token, var_dict, sub_query));
    REQUIRE(false == sub_query.wildcard_match_required());
    REQUIRE(1 == sub_query.get_num_possible_vars());
    auto const& var{sub_query.get_vars()[0]};
    REQUIRE(var.is_dict_var());
    REQUIRE(var.is_precise_var());
    REQUIRE(0 == var.get_var_dict_id());
    REQUIRE(var.get_possible_var_dict_ids().empty());
}

TEST_CASE("process_schema_encoded_non_greedy_wildcard_token ", "[dfa_search]") {
    FakeVarDict var_dict;
    var_dict.add_entry(0, "10a0");
    var_dict.add_entry(1, "10b0");

    SECTION("interpret_as_int") {
        SubQuery sub_query;
        VariableQueryToken const int_token{0, "10?0", true};
        REQUIRE(clp::GrepCoreTest::process_encoded_token(int_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(0 == sub_query.get_num_possible_vars());
    }

    SECTION("interpret_as_float") {
        SubQuery sub_query;
        VariableQueryToken const float_token{1, "10?0", true};
        REQUIRE(clp::GrepCoreTest::process_encoded_token(float_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(0 == sub_query.get_num_possible_vars());
    }

    SECTION("interpret_as_precise_has_number") {
        SubQuery sub_query;
        VariableQueryToken const has_number_token{2, "10a?", true};
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
        VariableQueryToken const has_number_token{2, "10?0", true};
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
    FakeVarDict var_dict;
    var_dict.add_entry(0, "100000000000000000000000010");
    var_dict.add_entry(1, "100000000000000000000000020");
    var_dict.add_entry(2, "100000000000000000000000030");
    var_dict.add_entry(3, "1000000000000000000000000.0");
    var_dict.add_entry(4, "1000000000000000000000000a0");

    SECTION("interpret_as_int") {
        SubQuery sub_query;
        VariableQueryToken const int_token{0, "1000000000000000000000000?0", true};
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
        VariableQueryToken const float_token{1, "1000000000000000000000000?0", true};
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
        VariableQueryToken const has_number_token{2, "1000000000000000000000000?0", true};
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
    FakeVarDict var_dict;
    var_dict.add_entry(0, "10a0");
    var_dict.add_entry(1, "10b0");
    var_dict.add_entry(2, "100000000000000000000000010");
    var_dict.add_entry(3, "100000000000000000000000020");
    var_dict.add_entry(4, "100000000000000000000000030");
    var_dict.add_entry(5, "1000000000000000000000000.0");
    var_dict.add_entry(6, "1000000000000000000000000a0");

    SECTION("interpret_as_non_encoded_int") {
        SubQuery sub_query;
        VariableQueryToken const int_token{0, "10*0", true};
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
        VariableQueryToken const float_token{0, "10*0", true};
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
        VariableQueryToken const has_number_token{0, "10*0", true};
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
        VariableQueryToken const has_number_token{0, "10b*", true};
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
        VariableQueryToken const int_token{0, "10*0", true};
        REQUIRE(clp::GrepCoreTest::process_encoded_token(int_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(0 == sub_query.get_num_possible_vars());
    }

    SECTION("interpret_as_encoded_float") {
        SubQuery sub_query;
        VariableQueryToken const float_token{1, "10*0", true};
        REQUIRE(clp::GrepCoreTest::process_encoded_token(float_token, var_dict, sub_query));
        REQUIRE(sub_query.wildcard_match_required());
        REQUIRE(0 == sub_query.get_num_possible_vars());
    }
}

// Tests: `generate_schema_sub_queries`
TEST_CASE("generate_schema_sub_queries", "[dfa_search]") {
    constexpr uint32_t cFloatId{static_cast<uint32_t>(TokenFloat)};
    constexpr uint32_t cHasNumberId{100};
    constexpr uint32_t cIntId{static_cast<uint32_t>(TokenInt)};

    FakeVarDict var_dict;
    var_dict.add_entry(0, "10a");
    var_dict.add_entry(1, "1a3");

    FakeLogTypeDict logtype_dict;

    string logtype_string{"static_text "};
    EncodedVariableInterpreter::add_int_var(logtype_string);
    logtype_string += " ";
    EncodedVariableInterpreter::add_int_var(logtype_string);
    logtype_string += " ";
    EncodedVariableInterpreter::add_float_var(logtype_string);
    logtype_dict.add_entry(logtype_string, 0);

    logtype_string = "static_text ";
    EncodedVariableInterpreter::add_int_var(logtype_string);
    logtype_string += " ";
    EncodedVariableInterpreter::add_dict_var(logtype_string);
    logtype_string += " ";
    EncodedVariableInterpreter::add_float_var(logtype_string);
    logtype_dict.add_entry(logtype_string, 1);

    logtype_string = "static_text ";
    EncodedVariableInterpreter::add_int_var(logtype_string);
    logtype_string += " ";
    EncodedVariableInterpreter::add_dict_var(logtype_string);
    logtype_string += " 3.14ab'";
    logtype_dict.add_entry(logtype_string, 2);

    logtype_string = "static_text ";
    EncodedVariableInterpreter::add_int_var(logtype_string);
    logtype_string += " ";
    EncodedVariableInterpreter::add_dict_var(logtype_string);
    logtype_string += " 3.15ab'";
    logtype_dict.add_entry(logtype_string, 3);

    logtype_string = "static_text ";
    EncodedVariableInterpreter::add_int_var(logtype_string);
    logtype_string += " 10' ";
    EncodedVariableInterpreter::add_float_var(logtype_string);
    logtype_dict.add_entry(logtype_string, 4);

    set<QueryInterpretation> interpretations;

    QueryInterpretation interpretation1{};
    interpretation1.append_static_token("static_text ");
    interpretation1.append_variable_token(cIntId, "100", false);
    interpretation1.append_static_token(" ");
    interpretation1.append_variable_token(cIntId, "10?", true);
    interpretation1.append_static_token(" ");
    interpretation1.append_variable_token(cFloatId, "3.14*", true);
    interpretations.insert(interpretation1);

    QueryInterpretation interpretation2{};
    interpretation2.append_static_token("static_text ");
    interpretation2.append_variable_token(cIntId, "100", false);
    interpretation2.append_static_token(" ");
    interpretation2.append_variable_token(cIntId, "10?", true);
    interpretation2.append_static_token(" ");
    interpretation2.append_variable_token(cHasNumberId, "3.14*", true);
    interpretations.insert(interpretation2);

    QueryInterpretation interpretation3{};
    interpretation3.append_static_token("static_text ");
    interpretation3.append_variable_token(cIntId, "100", false);
    interpretation3.append_static_token(" ");
    interpretation3.append_variable_token(cIntId, "10?", true);
    interpretation3.append_static_token(" 3.14*");
    interpretations.insert(interpretation3);

    QueryInterpretation interpretation4{};
    interpretation4.append_static_token("static_text ");
    interpretation4.append_variable_token(cIntId, "100", false);
    interpretation4.append_static_token(" ");
    interpretation4.append_variable_token(cHasNumberId, "10?", true);
    interpretation4.append_static_token(" ");
    interpretation4.append_variable_token(cFloatId, "3.14*", true);
    interpretations.insert(interpretation4);

    QueryInterpretation interpretation5{};
    interpretation5.append_static_token("static_text ");
    interpretation5.append_variable_token(cIntId, "100", false);
    interpretation5.append_static_token(" ");
    interpretation5.append_variable_token(cHasNumberId, "10?", true);
    interpretation5.append_static_token(" ");
    interpretation5.append_variable_token(cHasNumberId, "3.14*", true);
    interpretations.insert(interpretation5);

    QueryInterpretation interpretation6{};
    interpretation6.append_static_token("static_text ");
    interpretation6.append_variable_token(cIntId, "100", false);
    interpretation6.append_static_token(" ");
    interpretation6.append_variable_token(cHasNumberId, "10?", true);
    interpretation6.append_static_token(" 3.14*");
    interpretations.insert(interpretation6);

    QueryInterpretation interpretation7{};
    interpretation7.append_static_token("static_text ");
    interpretation7.append_variable_token(cIntId, "100", false);
    interpretation7.append_static_token(" 10? ");
    interpretation7.append_variable_token(cFloatId, "3.14*", true);
    interpretations.insert(interpretation7);

    QueryInterpretation interpretation8{};
    interpretation8.append_static_token("static_text ");
    interpretation8.append_variable_token(cIntId, "100", false);
    interpretation8.append_static_token(" 10? ");
    interpretation8.append_variable_token(cHasNumberId, "3.14*", true);
    interpretations.insert(interpretation8);

    QueryInterpretation interpretation9{};
    interpretation9.append_static_token("static_text ");
    interpretation9.append_variable_token(cIntId, "100", false);
    interpretation9.append_static_token(" 10? 3.14*");
    interpretations.insert(interpretation9);

    vector<SubQuery> sub_queries;
    clp::GrepCoreTest::generate_schema_sub_queries(
            interpretations,
            logtype_dict,
            var_dict,
            sub_queries
    );

    REQUIRE(6 == sub_queries.size());

    REQUIRE(sub_queries[0].wildcard_match_required());
    REQUIRE(2 == sub_queries[0].get_num_possible_vars());
    {
        auto const& var{sub_queries[0].get_vars()[0]};
        REQUIRE(false == var.is_dict_var());
        REQUIRE(var.is_precise_var());
    }
    {
        auto const& var{sub_queries[0].get_vars()[1]};
        REQUIRE(var.is_dict_var());
        REQUIRE(var.is_precise_var());
        REQUIRE(0 == var.get_var_dict_id());
        REQUIRE(var.get_possible_var_dict_ids().empty());
    }
    {
        auto logtype_ids{sub_queries[1].get_possible_logtypes()};
        REQUIRE(1 == logtype_ids.size());
        CAPTURE(logtype_ids);
        REQUIRE(logtype_ids.contains(0));
    }

    REQUIRE(sub_queries[1].wildcard_match_required());
    REQUIRE(1 == sub_queries[1].get_num_possible_vars());
    {
        auto const& var{sub_queries[1].get_vars()[0]};
        REQUIRE(false == var.is_dict_var());
        REQUIRE(var.is_precise_var());
    }
    {
        auto logtype_ids{sub_queries[1].get_possible_logtypes()};
        REQUIRE(1 == logtype_ids.size());
        CAPTURE(logtype_ids);
        REQUIRE(logtype_ids.contains(0));
    }

    REQUIRE(false == sub_queries[2].wildcard_match_required());
    REQUIRE(2 == sub_queries[2].get_num_possible_vars());
    {
        auto const& var{sub_queries[2].get_vars()[0]};
        REQUIRE(false == var.is_dict_var());
        REQUIRE(var.is_precise_var());
    }
    {
        auto const& var{sub_queries[2].get_vars()[1]};
        REQUIRE(var.is_dict_var());
        REQUIRE(var.is_precise_var());
    }
    {
        auto logtype_ids{sub_queries[2].get_possible_logtypes()};
        REQUIRE(1 == logtype_ids.size());
        CAPTURE(logtype_ids);
        REQUIRE(logtype_ids.contains(2));
    }

    REQUIRE(sub_queries[3].wildcard_match_required());
    REQUIRE(2 == sub_queries[3].get_num_possible_vars());
    {
        auto const& var{sub_queries[3].get_vars()[0]};
        REQUIRE(false == var.is_dict_var());
        REQUIRE(var.is_precise_var());
    }
    {
        auto const& var{sub_queries[3].get_vars()[1]};
        REQUIRE(var.is_dict_var());
        REQUIRE(var.is_precise_var());
    }
    {
        auto logtype_ids{sub_queries[3].get_possible_logtypes()};
        REQUIRE(1 == logtype_ids.size());
        CAPTURE(logtype_ids);
        REQUIRE(logtype_ids.contains(1));
    }

    REQUIRE(false == sub_queries[4].wildcard_match_required());
    REQUIRE(2 == sub_queries[4].get_num_possible_vars());
    {
        auto const& var{sub_queries[4].get_vars()[0]};
        REQUIRE(false == var.is_dict_var());
        REQUIRE(var.is_precise_var());
    }
    {
        auto const& var{sub_queries[4].get_vars()[1]};
        REQUIRE(var.is_dict_var());
        REQUIRE(var.is_precise_var());
    }
    {
        auto logtype_ids{sub_queries[4].get_possible_logtypes()};
        REQUIRE(1 == logtype_ids.size());
        CAPTURE(logtype_ids);
        REQUIRE(logtype_ids.contains(2));
    }

    REQUIRE(sub_queries[5].wildcard_match_required());
    REQUIRE(1 == sub_queries[5].get_num_possible_vars());
    {
        auto const& var{sub_queries[5].get_vars()[0]};
        REQUIRE(false == var.is_dict_var());
        REQUIRE(var.is_precise_var());
    }
    {
        auto logtype_ids{sub_queries[5].get_possible_logtypes()};
        REQUIRE(1 == logtype_ids.size());
        CAPTURE(logtype_ids);
        REQUIRE(logtype_ids.contains(4));
    }
}

/*
// Tests: `process_raw_query`
TEST_CASE("process_raw_query", "[dfa_search]") {
    constexpr uint32_t cFloatId{static_cast<uint32_t>(TokenFloat)};
    constexpr uint32_t cHasNumberId{100};
    constexpr uint32_t cIntId{static_cast<uint32_t>(TokenInt)};

    FakeVarDict var_dict;
    var_dict.add_entry(0, "10a");
    var_dict.add_entry(1, "1a3");

    FakeLogTypeDict logtype_dict;

    string logtype_string{"static_text "};
    EncodedVariableInterpreter::add_int_var(logtype_string);
    logtype_string += " ";
    EncodedVariableInterpreter::add_int_var(logtype_string);
    logtype_string += " ";
    EncodedVariableInterpreter::add_float_var(logtype_string);
    logtype_dict.add_entry(logtype_string, 0);

    logtype_string = "static_text ";
    EncodedVariableInterpreter::add_int_var(logtype_string);
    logtype_string += " ";
    EncodedVariableInterpreter::add_dict_var(logtype_string);
    logtype_string += " ";
    EncodedVariableInterpreter::add_float_var(logtype_string);
    logtype_dict.add_entry(logtype_string, 0);

    logtype_string = "static_text ";
    EncodedVariableInterpreter::add_int_var(logtype_string);
    logtype_string += " ";
    EncodedVariableInterpreter::add_dict_var(logtype_string);
    logtype_string += " 3.14ab'";
    logtype_dict.add_entry(logtype_string, 0);

    logtype_string = "static_text ";
    EncodedVariableInterpreter::add_int_var(logtype_string);
    logtype_string += " ";
    EncodedVariableInterpreter::add_dict_var(logtype_string);
    logtype_string += " 3.15ab'";
    logtype_dict.add_entry(logtype_string, 0);

    logtype_string = "static_text ";
    EncodedVariableInterpreter::add_int_var(logtype_string);
    logtype_string += " 10' ";
    EncodedVariableInterpreter::add_float_var(logtype_string);
    logtype_dict.add_entry(logtype_string, 0);

    string raw_query{"static_text 100 10? 3.14*"};

    auto const query{GrepCore::process_raw_query(
            logtype_dict,
            var_dict,
            raw_query,
            0,
            0,
            false,
            lexer,
            false
    )};

    auto const& sub_queries{query.get_sub_queries()};
    REQUIRE(6 == sub_queries.size());
}
*/

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
