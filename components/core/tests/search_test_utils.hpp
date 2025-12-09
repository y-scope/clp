#ifndef SEARCH_TEST_UTILS_HPP
#define SEARCH_TEST_UTILS_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "../src/clp/Defs.h"
#include "../src/clp/EncodedVariableInterpreter.hpp"
#include "../src/clp/Query.hpp"
#include "MockLogTypeDictionary.hpp"
#include "MockVariableDictionary.hpp"

using clp::EncodedVariableInterpreter;
using clp::logtype_dictionary_id_t;
using std::pair;
using std::string;
using std::string_view;
using clp::SubQuery;
using std::tuple;
using std::unordered_set;
using std::variant;
using std::vector;
using clp::variable_dictionary_id_t;

using VarInfo = tuple<bool, bool, unordered_set<variable_dictionary_id_t>>;

/**
 * @param entries Vector of (id, value) pairs to populate the variable
 * dictionary.
 * @return A `MockVarDictionary` initialized with the given entries.
 */
auto make_var_dict(vector<pair<size_t, string>> const& entries) -> MockVarDictionary;

/**
 * @param entries Vector of logtypes, where each logtype is represented by a vector of tokens. Each
 * token is either a literal substring (`string_view`) or a variable placeholder (`char`).
 * @return A `MockLogTypeDictionary` initialized with the given entries.
 */
auto make_logtype_dict(vector<vector<variant<string_view, char>>> const& entries)
        -> MockLogTypeDictionary;

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
        vector<VarInfo> const& vars_info,
        unordered_set<logtype_dictionary_id_t> const& logtype_ids
) -> void;

#endif  // SEARCH_TEST_UTILS_HPP
