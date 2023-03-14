#include "CompositeWildcardToken.hpp"

using std::string;
using std::string_view;
using std::variant;
using std::vector;

auto TokenGetBeginPos = [] (const auto& token) { return token.get_begin_pos(); };
auto TokenGetEndPos = [] (const auto& token) { return token.get_end_pos(); };

namespace ffi::search {
    template <typename encoded_variable_t>
    CompositeWildcardToken<encoded_variable_t>::CompositeWildcardToken (
            string_view query, size_t begin_pos, size_t end_pos
    ) : QueryToken(query, begin_pos, end_pos) {
        // Find wildcards
        bool is_escaped = false;
        for (size_t i = begin_pos; i < end_pos; ++i) {
            auto c = query[i];

            if (is_escaped) {
                is_escaped = false;
            } else if ('\\' == c) {
                is_escaped = true;
            } else if (is_wildcard(c)) {
                m_wildcards.emplace_back(c, i, begin_pos == i || end_pos - 1 == i);
            }
        }
        if (m_wildcards.empty()) {
            throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
        }

        tokenize_into_wildcard_variable_tokens();
    }

    template <typename encoded_variable_t>
    void CompositeWildcardToken<encoded_variable_t>::add_to_query (
            string& logtype_query,
            vector<variant<ExactVariableToken<encoded_variable_t>,
                    WildcardToken<encoded_variable_t>>>& variable_tokens
    ) const {
        // We need to handle '*' carefully when building the logtype query since
        // we may have a token like "a1*b2" with interpretation ["a1*", "*b2"].
        // In this case, we want to make sure the logtype query only ends up
        // with one '*' rather than one for the suffix of "a1*" and one for the
        // prefix of "*b2". So the algorithm below only adds a '*" to the
        // logtype query if the current variable has a prefix '*'. Then after
        // the loop, if the last variable had a suffix '*', we add a '*' to the
        // logtype query before adding any remaining query content.
        auto constant_begin_pos = m_begin_pos;
        for (const auto& wildcard_var : m_wildcard_variables) {
            auto begin_pos = std::visit(TokenGetBeginPos, wildcard_var);
            // Copy from the end of the last variable to the beginning of this
            // one (if this wildcard variable doesn't overlap with the previous
            // one)
            if (begin_pos > constant_begin_pos) {
                logtype_query.append(m_query, constant_begin_pos, begin_pos - constant_begin_pos);
            }
            std::visit(overloaded{
                    [&logtype_query, &variable_tokens] (
                            const ExactVariableToken<encoded_variable_t>& var
                    ) {
                        var.add_to_logtype_query(logtype_query);
                        variable_tokens.emplace_back(var);
                    },
                    [&logtype_query, &variable_tokens] (
                            const WildcardToken<encoded_variable_t>& var
                    ) {
                        if (var.add_to_logtype_query(logtype_query)) {
                            variable_tokens.emplace_back(var);
                        }
                    }
            }, wildcard_var);
            constant_begin_pos = std::visit(TokenGetEndPos, wildcard_var);
        }
        // Add the remainder
        if (false == m_wildcard_variables.empty()) {
            const auto& last_var = m_wildcard_variables.back();
            if (std::holds_alternative<WildcardToken<encoded_variable_t>>(last_var)) {
                const auto& wildcard_var =
                        std::get<WildcardToken<encoded_variable_t>>(last_var);
                if (wildcard_var.has_suffix_star_wildcard()) {
                    logtype_query += enum_to_underlying_type(WildcardType::ZeroOrMoreChars);
                }
            }
        }
        logtype_query.append(m_query, constant_begin_pos, m_end_pos - constant_begin_pos);
    }

    template <typename encoded_variable_t>
    bool CompositeWildcardToken<encoded_variable_t>::generate_next_interpretation () {
        for (auto& w : m_wildcard_variables) {
            if (std::holds_alternative<WildcardToken<encoded_variable_t>>(w)) {
                auto& wildcard_var = std::get<WildcardToken<encoded_variable_t>>(w);
                if (wildcard_var.next_interpretation()) {
                    return true;
                }
            }
        }

        for (auto& w : m_wildcards) {
            if (w.next_interpretation()) {
                tokenize_into_wildcard_variable_tokens();
                return true;
            }
        }

        return false;
    }

    /**
     * The algorithm works as follows:
     * - Each '*' at the edge of the token has one interpretation:
     *   1. matching a combination of non-delimiters and delimiters.
     * - Every other '*' has two interpretations:
     *   1. matching a combination of non-delimiters and delimiters, or
     *   2. only matching non-delimiters.
     * - Each '?' has two interpretations:
     *   1. matching a non-delimiter, or
     *   2. matching a delimiter.
     * - When generating a token interpretation, if none of the wildcards can
     *   match a delimiter, then the interpretation is simply the entire token.
     * - However, if one of the wildcards can match a delimiter, then the token
     *   splits into two at the delimiter.
     * - For example, consider the token "*abc*def?hij?":
     *   - If just the first '?' is interpreted as matching a delimiter, then
     *     the token-interpretation becomes ["*abc*def", "hij?"].
     *   - So the algorithm looks for each of these wildcards matching
     *     delimiters and then creates a token from the content bounded by the
     *     wildcards.
     *
     * NOTE: We could cache wildcard variables that we generate (using their
     * bounds in the query as the cache key) so that we don't end up
     * regenerating them in other tokenizations. This isn't a performance
     * problem now, but could be an issue if we need to search the variable
     * dictionary for each generated WildcardToken.
     */
    template <typename encoded_variable_t>
    void CompositeWildcardToken<encoded_variable_t>::tokenize_into_wildcard_variable_tokens ()  {
        m_wildcard_variables.clear();

        const QueryWildcard* last_wildcard = nullptr;
        bool wildcardInVariable = false;
        size_t var_begin_pos, var_end_pos;
        for (const auto& w : m_wildcards) {
            switch (w.get_current_interpretation()) {
                case WildcardInterpretation::NoDelimiters:
                    wildcardInVariable = true;
                    break;
                case WildcardInterpretation::ContainsDelimiters: {
                    auto wildcard_pos = w.get_pos_in_query();
                    if (wildcard_pos == m_begin_pos) {
                        last_wildcard = &w;
                        // Nothing to do yet since wildcard is at the beginning
                        // of the token
                        continue;
                    }

                    // Determine var_begin_pos
                    if (nullptr == last_wildcard) {
                        var_begin_pos = m_begin_pos;
                    } else {
                        if (WildcardType::ZeroOrMoreChars == last_wildcard->get_type()) {
                            // Include the wildcard in the token
                            var_begin_pos = last_wildcard->get_pos_in_query();
                            wildcardInVariable = true;
                        } else {
                            var_begin_pos = last_wildcard->get_pos_in_query() + 1;
                        }
                    }

                    // Determine var_end_pos
                    if (WildcardType::ZeroOrMoreChars == w.get_type()) {
                        // Include the wildcard in the token
                        var_end_pos = wildcard_pos + 1;
                        wildcardInVariable = true;
                    } else {
                        var_end_pos = wildcard_pos;
                    }

                    if (wildcardInVariable) {
                        m_wildcard_variables.emplace_back(
                                std::in_place_type<WildcardToken<encoded_variable_t>>,
                                m_query,
                                var_begin_pos, var_end_pos);
                    } else {
                        string_view var(m_query.cbegin() + var_begin_pos,
                                        var_end_pos - var_begin_pos);
                        if (is_var(var)) {
                            m_wildcard_variables.emplace_back(
                                    std::in_place_type<ExactVariableToken<encoded_variable_t>>,
                                    m_query, var_begin_pos, var_end_pos);
                        }
                    }

                    last_wildcard = &w;
                    wildcardInVariable = false;
                    break;
                }
                default:
                    throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
            }
        }

        if (nullptr == last_wildcard) {
            // NOTE: Since the token contains a wildcard (this is the
            // CompositeWildcardToken class), there's no way this could be an
            // ExactVariableToken
            m_wildcard_variables.emplace_back(
                    std::in_place_type<WildcardToken<encoded_variable_t>>, m_query,
                    m_begin_pos, m_end_pos);
        } else if (last_wildcard->get_pos_in_query() < m_end_pos - 1) {
            if (WildcardType::ZeroOrMoreChars == last_wildcard->get_type()) {
                // Include the wildcard in the token
                var_begin_pos = last_wildcard->get_pos_in_query();
                wildcardInVariable = true;
            } else {
                var_begin_pos = last_wildcard->get_pos_in_query() + 1;
            }

            var_end_pos = m_end_pos;

            if (wildcardInVariable) {
                m_wildcard_variables.emplace_back(
                        std::in_place_type<WildcardToken<encoded_variable_t>>,
                        m_query,
                        var_begin_pos, var_end_pos);
            } else {
                string_view var(m_query.cbegin() + var_begin_pos, var_end_pos - var_begin_pos);
                if (is_var(var)) {
                    m_wildcard_variables.emplace_back(
                            std::in_place_type<ExactVariableToken<encoded_variable_t>>, m_query,
                            var_begin_pos, var_end_pos);
                }
            }
        }
    }

    // Explicitly declare specializations to avoid having to validate that the
    // template parameters are supported
    template class ffi::search::CompositeWildcardToken<ffi::eight_byte_encoded_variable_t>;
    template class ffi::search::CompositeWildcardToken<ffi::four_byte_encoded_variable_t>;
}
