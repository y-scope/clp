#include "query_methods.hpp"

#include <string_utils/string_utils.hpp>

#include "../../ir/parsing.hpp"
#include "../../ir/types.hpp"
#include "CompositeWildcardToken.hpp"
#include "QueryMethodFailed.hpp"

using clp::string_utils::is_wildcard;
using glt::ir::eight_byte_encoded_variable_t;
using glt::ir::four_byte_encoded_variable_t;
using glt::ir::is_delim;
using std::pair;
using std::string;
using std::string_view;
using std::variant;
using std::vector;

namespace glt::ffi::search {
static auto TokenGetBeginPos = [](auto const& token) { return token.get_begin_pos(); };
static auto TokenGetEndPos = [](auto const& token) { return token.get_end_pos(); };

/**
 * Finds the next delimiter that's not also a wildcard
 * @param value
 * @param pos Position to the start the search from, returns the position of the delimiter (if
 * found)
 * @param contains_alphabet Returns whether the string contains an alphabet
 * @param contains_decimal_digit Returns whether the string contains a decimal digit
 * @param contains_wildcard Returns whether the string contains a wildcard
 */
static void find_delimiter(
        string_view value,
        size_t& pos,
        bool& contains_alphabet,
        bool& contains_decimal_digit,
        bool& contains_wildcard
);
/**
 * Finds the next wildcard or non-delimiter in the given string, starting from the given position
 * @param value
 * @param pos Position to the start the search from, returns the position of the wildcard or
 * non-delimiter (if found)
 * @param contains_wildcard Returns whether the string contains a wildcard
 * @return Whether a wildcard/non-delimiter was found
 */
static bool find_wildcard_or_non_delimiter(string_view value, size_t& pos, bool& contains_wildcard);

/**
 * Tokenizes the given wildcard query into exact variables (as would be found by
 * ffi::get_bounds_of_next_var) and potential variables, i.e., any token with a wildcard.
 * @tparam encoded_variable_t Type for encoded variable values
 * @param wildcard_query
 * @param tokens
 * @param composite_wildcard_token_indexes Indexes of the tokens in \p tokens which contain
 * wildcards
 */
template <typename encoded_variable_t>
static void tokenize_query(
        string_view wildcard_query,
        vector<
                variant<ExactVariableToken<encoded_variable_t>,
                        CompositeWildcardToken<encoded_variable_t>>>& tokens,
        vector<size_t>& composite_wildcard_token_indexes
);

template <typename encoded_variable_t>
void
generate_subqueries(string_view wildcard_query, vector<Subquery<encoded_variable_t>>& sub_queries) {
    if (wildcard_query.empty()) {
        throw QueryMethodFailed(
                ErrorCode_BadParam,
                __FILENAME__,
                __LINE__,
                "wildcard_query cannot be empty"
        );
    }

    vector<
            variant<ExactVariableToken<encoded_variable_t>,
                    CompositeWildcardToken<encoded_variable_t>>>
            tokens;
    vector<size_t> composite_wildcard_token_indexes;
    tokenize_query(wildcard_query, tokens, composite_wildcard_token_indexes);

    bool all_interpretations_complete = false;
    auto escape_handler
            = [](string_view constant, size_t char_to_escape_pos, string& logtype) -> void {
        auto const next_char_pos{char_to_escape_pos + 1};
        // NOTE: We don't want to add additional escapes for wildcards that have been escaped. E.g.,
        // the query "\\*" should remain unchanged.
        if (ir::is_variable_placeholder(constant[char_to_escape_pos])
            || (next_char_pos < constant.length() && false == is_wildcard(constant[next_char_pos])))
        {
            logtype += enum_to_underlying_type(ir::VariablePlaceholder::Escape);
        }
    };
    string logtype_query;
    vector<variant<ExactVariableToken<encoded_variable_t>, WildcardToken<encoded_variable_t>>>
            query_vars;
    while (false == all_interpretations_complete) {
        logtype_query.clear();
        query_vars.clear();
        size_t constant_begin_pos = 0;
        for (auto const& token : tokens) {
            auto begin_pos = std::visit(TokenGetBeginPos, token);
            ir::append_constant_to_logtype(
                    wildcard_query.substr(constant_begin_pos, begin_pos - constant_begin_pos),
                    escape_handler,
                    logtype_query
            );

            std::visit(
                    overloaded{
                            [&logtype_query, &query_vars](  // clang-format off
                                    ExactVariableToken<encoded_variable_t> const& token
                            ) {  // clang-format on
                                token.add_to_logtype_query(logtype_query);
                                query_vars.emplace_back(token);
                            },
                            [&logtype_query, &query_vars](  // clang-format off
                                    CompositeWildcardToken<encoded_variable_t> const& token
                            ) {  // clang-format on
                                token.add_to_query(logtype_query, query_vars);
                            }
                    },
                    token
            );

            constant_begin_pos = std::visit(TokenGetEndPos, token);
        }
        ir::append_constant_to_logtype(
                wildcard_query.substr(constant_begin_pos),
                escape_handler,
                logtype_query
        );

        // Save sub-query if it's unique
        bool sub_query_exists = false;
        for (auto const& sub_query : sub_queries) {
            if (sub_query.equals(logtype_query, query_vars)) {
                sub_query_exists = true;
                break;
            }
        }
        if (false == sub_query_exists) {
            sub_queries.emplace_back(logtype_query, query_vars);
        }

        // Generate next interpretation if any
        all_interpretations_complete = true;
        for (auto i : composite_wildcard_token_indexes) {
            auto& w = std::get<CompositeWildcardToken<encoded_variable_t>>(tokens[i]);
            if (w.generate_next_interpretation()) {
                all_interpretations_complete = false;
                break;
            }
        }
    }
}

template <typename encoded_variable_t>
void tokenize_query(
        string_view wildcard_query,
        vector<
                variant<ExactVariableToken<encoded_variable_t>,
                        CompositeWildcardToken<encoded_variable_t>>>& tokens,
        vector<size_t>& composite_wildcard_token_indexes
) {
    // Tokenize query using delimiters to get definite variables and tokens containing wildcards
    // (potential variables)
    size_t end_pos = 0;
    while (true) {
        auto begin_pos = end_pos;

        bool contains_wildcard;
        if (false == find_wildcard_or_non_delimiter(wildcard_query, begin_pos, contains_wildcard)) {
            break;
        }

        bool contains_decimal_digit = false;
        bool contains_alphabet = false;
        end_pos = begin_pos;
        find_delimiter(
                wildcard_query,
                end_pos,
                contains_alphabet,
                contains_decimal_digit,
                contains_wildcard
        );

        if (contains_wildcard) {
            // Only consider tokens which contain more than just a wildcard
            if (end_pos - begin_pos > 1) {
                tokens.emplace_back(
                        std::in_place_type<CompositeWildcardToken<encoded_variable_t>>,
                        wildcard_query,
                        begin_pos,
                        end_pos
                );
                composite_wildcard_token_indexes.push_back(tokens.size() - 1);
            }
        } else {
            string_view variable(wildcard_query.cbegin() + begin_pos, end_pos - begin_pos);
            // Treat token as variable if:
            // - it contains a decimal digit, or
            // - it's directly preceded by an equals sign and contains an alphabet, or
            // - it could be a multi-digit hex value
            if (contains_decimal_digit
                || (begin_pos > 0 && '=' == wildcard_query[begin_pos - 1] && contains_alphabet)
                || ir::could_be_multi_digit_hex_value(variable))
            {
                tokens.emplace_back(
                        std::in_place_type<ExactVariableToken<encoded_variable_t>>,
                        wildcard_query,
                        begin_pos,
                        end_pos
                );
            }
        }
    }
}

static void find_delimiter(
        string_view value,
        size_t& pos,
        bool& contains_alphabet,
        bool& contains_decimal_digit,
        bool& contains_wildcard
) {
    bool is_escaped = false;
    for (; pos < value.length(); ++pos) {
        auto c = value[pos];

        if (is_escaped) {
            is_escaped = false;

            if (is_delim(c)) {
                // Found escaped delimiter, so reverse the index to exclude the escape character
                --pos;
                return;
            }
        } else if ('\\' == c) {
            is_escaped = true;
        } else {
            if (is_wildcard(c)) {
                contains_wildcard = true;
            } else if (is_delim(c)) {
                // Found delimiter that's not also a wildcard
                return;
            }
        }

        if (clp::string_utils::is_decimal_digit(c)) {
            contains_decimal_digit = true;
        } else if (clp::string_utils::is_alphabet(c)) {
            contains_alphabet = true;
        }
    }
}

static bool
find_wildcard_or_non_delimiter(string_view value, size_t& pos, bool& contains_wildcard) {
    bool is_escaped = false;
    contains_wildcard = false;
    for (; pos < value.length(); ++pos) {
        auto c = value[pos];

        if (is_escaped) {
            is_escaped = false;

            if (false == is_delim(c)) {
                // Found escaped non-delimiter, so reverse the index to retain the escape character
                --pos;
                return true;
            }
        } else if ('\\' == c) {
            is_escaped = true;
        } else {
            if (is_wildcard(c)) {
                contains_wildcard = true;
                return true;
            } else if (false == is_delim(c)) {
                return true;
            }
        }
    }

    return false;
}

// Explicitly declare specializations to avoid having to validate that the template parameters are
// supported
template void generate_subqueries<eight_byte_encoded_variable_t>(
        string_view wildcard_query,
        vector<Subquery<eight_byte_encoded_variable_t>>& sub_queries
);
template void generate_subqueries<four_byte_encoded_variable_t>(
        string_view wildcard_query,
        vector<Subquery<four_byte_encoded_variable_t>>& sub_queries
);
template void tokenize_query<eight_byte_encoded_variable_t>(
        string_view wildcard_query,
        vector<
                variant<ExactVariableToken<eight_byte_encoded_variable_t>,
                        CompositeWildcardToken<eight_byte_encoded_variable_t>>>& tokens,
        vector<size_t>& composite_wildcard_token_indexes
);
template void tokenize_query<four_byte_encoded_variable_t>(
        string_view wildcard_query,
        vector<
                variant<ExactVariableToken<four_byte_encoded_variable_t>,
                        CompositeWildcardToken<four_byte_encoded_variable_t>>>& tokens,
        vector<size_t>& composite_wildcard_token_indexes
);
}  // namespace glt::ffi::search
