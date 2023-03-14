#include "query_methods.hpp"

// Project headers
#include "../../string_utils.hpp"
#include "CompositeWildcardToken.hpp"
#include "QueryMethodFailed.hpp"

using std::pair;
using std::string;
using std::string_view;
using std::variant;
using std::vector;

auto TokenGetBeginPos = [] (const auto& token) { return token.get_begin_pos(); };
auto TokenGetEndPos = [] (const auto& token) { return token.get_end_pos(); };

namespace ffi::search {
    /**
     * Tokenizes the given wildcard query into exact variables (as would be
     * found by ffi::get_bounds_of_next_var) and potential variables, i.e., any
     * token with a wildcard.
     * @tparam encoded_variable_t Type for encoded variable values
     * @param wildcard_query
     * @param tokens
     * @param composite_wildcard_tokens Pointers to tokens in @p tokens which
     * contain wildcards
     */
    template <typename encoded_variable_t>
    static void tokenize_query (
            string_view wildcard_query,
            vector<variant<ExactVariableToken<encoded_variable_t>,
                    CompositeWildcardToken<encoded_variable_t>>>& tokens,
            vector<CompositeWildcardToken<encoded_variable_t>*>& composite_wildcard_tokens
    );

    template<typename encoded_variable_t>
    void generate_subqueries (
            const string& wildcard_query,
            vector<
                    pair<
                            string,
                            vector<
                                    variant<
                                            ExactVariableToken<encoded_variable_t>,
                                            WildcardToken<encoded_variable_t>
                                    >
                            >
                    >
            >& sub_queries
    ) {
        if (wildcard_query.empty()) {
            throw QueryMethodFailed(ErrorCode_BadParam, __FILENAME__, __LINE__,
                                    "wildcard_query cannot be empty");
        }

        vector<variant<ExactVariableToken<encoded_variable_t>,
                CompositeWildcardToken<encoded_variable_t>>> tokens;
        vector<CompositeWildcardToken<encoded_variable_t>*> composite_wildcard_tokens;
        tokenize_query(wildcard_query, tokens, composite_wildcard_tokens);

        string logtype_query;
        vector<variant<ExactVariableToken<encoded_variable_t>,
                WildcardToken<encoded_variable_t>>> query_vars;
        while (true) {
            logtype_query.clear();
            query_vars.clear();
            size_t last_variable_end_pos = 0;
            for (const auto& token : tokens) {
                auto begin_pos = std::visit(TokenGetBeginPos, token);
                logtype_query.append(wildcard_query, last_variable_end_pos,
                                     begin_pos - last_variable_end_pos);

                std::visit(overloaded{
                        [&logtype_query, &query_vars] (
                                const ExactVariableToken<encoded_variable_t>& token
                        ) {
                            token.add_to_logtype_query(logtype_query);
                            query_vars.emplace_back(token);
                        },
                        [&logtype_query, &query_vars] (
                                const CompositeWildcardToken<encoded_variable_t>& token
                        ) {
                            token.add_to_query(logtype_query, query_vars);
                        }
                }, token);

                last_variable_end_pos = std::visit(TokenGetEndPos, token);
            }
            logtype_query.append(wildcard_query, last_variable_end_pos);

            // Save sub-query if it's unique
            bool sub_query_exists = false;
            for (const auto& sub_query : sub_queries) {
                const auto& sub_query_logtype_query = sub_query.first;
                const auto& sub_query_query_vars = sub_query.second;
                if (sub_query_logtype_query == logtype_query
                    && sub_query_query_vars == query_vars)
                {
                    sub_query_exists = true;
                    break;
                }
            }
            if (false == sub_query_exists) {
                sub_queries.emplace_back(logtype_query, query_vars);
            }

            // Generate next interpretation if any
            bool all_interpretations_complete = true;
            for (auto w : composite_wildcard_tokens) {
                if (w->generate_next_interpretation()) {
                    all_interpretations_complete = false;
                    break;
                }
            }
            if (all_interpretations_complete) {
                break;
            }
        }
    }

    template <typename encoded_variable_t>
    void tokenize_query (
            string_view wildcard_query,
            vector <variant<ExactVariableToken<encoded_variable_t>,
                    CompositeWildcardToken<encoded_variable_t>>>& tokens,
            vector<CompositeWildcardToken<encoded_variable_t>*>& composite_wildcard_tokens
    ) {
        // Tokenize query using delimiters to get definite variables and tokens
        // containing wildcards (potential variables)
        size_t end_pos = 0;
        auto wildcard_query_length = wildcard_query.length();
        while (true) {
            auto begin_pos = end_pos;

            // Find next wildcard or delimiter
            bool is_escaped = false;
            bool contains_wildcard = false;
            for (; begin_pos < wildcard_query_length; ++begin_pos) {
                auto c = wildcard_query[begin_pos];

                if (is_escaped) {
                    is_escaped = false;

                    if (false == is_delim(c)) {
                        // Found escaped non-delimiter, so reverse the index to
                        // retain the escape character
                        --begin_pos;
                        break;
                    }
                } else if ('\\' == c) {
                    is_escaped = true;
                } else {
                    if (is_wildcard(c)) {
                        contains_wildcard = true;
                        break;
                    } else if (false == is_delim(c)) {
                        break;
                    }
                }
            }
            if (wildcard_query_length == begin_pos) {
                // Early exit for performance
                break;
            }

            bool contains_decimal_digit = false;
            bool contains_alphabet = false;

            // Find next delimiter
            is_escaped = false;
            end_pos = begin_pos;
            while (end_pos < wildcard_query_length) {
                auto c = wildcard_query[end_pos];

                if (is_escaped) {
                    is_escaped = false;

                    if (is_delim(c)) {
                        // Found escaped delimiter, so reverse the index to
                        // exclude the escape character
                        --end_pos;
                        break;
                    }
                } else if ('\\' == c) {
                    is_escaped = true;
                } else {
                    if (is_wildcard(c)) {
                        contains_wildcard = true;
                    } else if (is_delim(c)) {
                        // Found delimiter that's not also a wildcard
                        break;
                    }
                }

                if (is_decimal_digit(c)) {
                    contains_decimal_digit = true;
                } else if (is_alphabet(c)) {
                    contains_alphabet = true;
                }

                ++end_pos;
            }

            if (contains_wildcard) {
                // Only consider tokens which contain more than just a wildcard
                if (end_pos - begin_pos > 1) {
                    auto& token = tokens.emplace_back(
                            std::in_place_type<CompositeWildcardToken<encoded_variable_t>>,
                            wildcard_query,
                            begin_pos, end_pos);
                    composite_wildcard_tokens.push_back(
                            &std::get<CompositeWildcardToken<encoded_variable_t>>(token));
                }
            } else {
                string_view variable(wildcard_query.cbegin() + begin_pos, end_pos - begin_pos);
                // Treat token as variable if:
                // - it contains a decimal digit, or
                // - it's directly preceded by an equals sign and contains an
                //   alphabet, or
                // - it could be a multi-digit hex value
                if (contains_decimal_digit ||
                    (begin_pos > 0 && '=' == wildcard_query[begin_pos - 1] && contains_alphabet) ||
                    ffi::could_be_multi_digit_hex_value(variable)) {
                    tokens.emplace_back(
                            std::in_place_type<ExactVariableToken<encoded_variable_t>>,
                            wildcard_query, begin_pos, end_pos);
                }
            }
        }
    }

    // Explicitly declare specializations to avoid having to validate that the
    // template parameters are supported
    template void generate_subqueries (
            const string& wildcard_query,
            vector<
                    pair<
                            string,
                            vector<
                                    variant<
                                            ExactVariableToken<eight_byte_encoded_variable_t>,
                                            WildcardToken<eight_byte_encoded_variable_t>
                                    >
                            >
                    >
            >& sub_queries
    );
    template void generate_subqueries (
            const string& wildcard_query,
            vector<
                    pair<
                            string,
                            vector<
                                    variant<
                                            ExactVariableToken<four_byte_encoded_variable_t>,
                                            WildcardToken<four_byte_encoded_variable_t>
                                    >
                            >
                    >
            >& sub_queries
    );
    template void tokenize_query<ffi::eight_byte_encoded_variable_t> (
            string_view wildcard_query,
            vector<variant<ExactVariableToken<eight_byte_encoded_variable_t>,
                    CompositeWildcardToken<eight_byte_encoded_variable_t>>>& tokens,
            vector<CompositeWildcardToken<
                    eight_byte_encoded_variable_t>*>& composite_wildcard_tokens
    );
    template void tokenize_query<ffi::four_byte_encoded_variable_t>(
            string_view wildcard_query,
            vector<variant<ExactVariableToken<four_byte_encoded_variable_t>,
                    CompositeWildcardToken<four_byte_encoded_variable_t>>>& tokens,
            vector<CompositeWildcardToken<
                    four_byte_encoded_variable_t>*>& composite_wildcard_tokens
    );

}

