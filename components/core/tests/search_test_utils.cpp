#include "search_test_utils.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <clp/Defs.h>
#include <clp/EncodedVariableInterpreter.hpp>
#include <clp/Query.hpp>

#include "MockLogTypeDictionary.hpp"
#include "MockVariableDictionary.hpp"

using clp::EncodedVariableInterpreter;
using clp::logtype_dictionary_id_t;
using clp::SubQuery;
using clp::variable_dictionary_id_t;
using std::pair;
using std::string;
using std::string_view;
using std::tuple;
using std::unordered_set;
using std::variant;
using std::vector;

auto make_var_dict(vector<pair<variable_dictionary_id_t, string>> const& entries)
        -> MockVariableDictionary {
    MockVariableDictionary dict;
    for (auto const& [id, val] : entries) {
        dict.add_entry(id, val);
    }
    return dict;
}

auto make_logtype_dict(vector<vector<variant<string_view, char>>> const& entries)
        -> MockLogTypeDictionary {
    MockLogTypeDictionary dict;
    logtype_dictionary_id_t id{0};
    for (auto const& entry : entries) {
        dict.add_entry(generate_expected_logtype_string(entry), id++);
    }
    return dict;
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
                    REQUIRE(false);
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
        vector<VarInfo> const& vars_info,
        unordered_set<logtype_dictionary_id_t> const& logtype_ids
) -> void {
    CAPTURE(id);
    REQUIRE(id < sub_queries.size());
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
