#include "CompositeWildcardToken.hpp"

#include <string_utils/string_utils.hpp>

#include "../../ir/parsing.hpp"
#include "../../ir/types.hpp"

using std::string;
using std::string_view;
using std::variant;
using std::vector;

namespace clp::ffi::search {
static auto TokenGetBeginPos = [](auto const& token) { return token.get_begin_pos(); };
static auto TokenGetEndPos = [](auto const& token) { return token.get_end_pos(); };

template <typename encoded_variable_t>
CompositeWildcardToken<encoded_variable_t>::CompositeWildcardToken(
        string_view query,
        size_t begin_pos,
        size_t end_pos
)
        : QueryToken(query, begin_pos, end_pos) {
    // Find wildcards
    bool is_escaped = false;
    for (size_t i = begin_pos; i < end_pos; ++i) {
        auto c = query[i];

        if (is_escaped) {
            is_escaped = false;
        } else if ('\\' == c) {
            is_escaped = true;
        } else if (string_utils::is_wildcard(c)) {
            m_wildcards.emplace_back(c, i, begin_pos == i || end_pos - 1 == i);
        }
    }
    if (m_wildcards.empty()) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }

    tokenize_into_wildcard_variable_tokens();
}

template <typename encoded_variable_t>
void CompositeWildcardToken<encoded_variable_t>::add_to_query(
        string& logtype_query,
        vector<variant<ExactVariableToken<encoded_variable_t>, WildcardToken<encoded_variable_t>>>&
                variable_tokens
) const {
    // We need to handle '*' carefully when building the logtype query since we may have a token
    // like "a1*b2" with interpretation ["a1*", "*b2"]. In this case, we want to make sure the
    // logtype query only ends up with one '*' rather than one for the suffix of "a1*" and one for
    // the prefix of "*b2". So the algorithm below only adds a '*' to the logtype query if the
    // current variable has a prefix '*' (i.e., we ignore suffix '*'). Then after the loop, if the
    // last variable had a suffix '*', we add a '*' to the logtype query before adding any remaining
    // query content.
    auto constant_begin_pos = m_begin_pos;
    for (auto const& var : m_variables) {
        auto begin_pos = std::visit(TokenGetBeginPos, var);
        // Copy from the end of the last variable to the beginning of this one (if this wildcard
        // variable doesn't overlap with the previous one)
        if (begin_pos > constant_begin_pos) {
            logtype_query.append(m_query, constant_begin_pos, begin_pos - constant_begin_pos);
        }
        std::visit(
                overloaded{
                        [&logtype_query, &variable_tokens](  // clang-format off
                                ExactVariableToken<encoded_variable_t> const& exact_var
                        ) {  // clang-format on
                            exact_var.add_to_logtype_query(logtype_query);
                            variable_tokens.emplace_back(exact_var);
                        },
                        [&logtype_query, &variable_tokens](  // clang-format off
                                WildcardToken<encoded_variable_t> const& wildcard_var
                        ) {  // clang-format on
                            if (wildcard_var.add_to_logtype_query(logtype_query)) {
                                variable_tokens.emplace_back(wildcard_var);
                            }
                        }
                },
                var
        );
        constant_begin_pos = std::visit(TokenGetEndPos, var);
    }
    // Add the remainder
    if (false == m_variables.empty()) {
        auto const& last_var = m_variables.back();
        if (std::holds_alternative<WildcardToken<encoded_variable_t>>(last_var)) {
            auto const& wildcard_var = std::get<WildcardToken<encoded_variable_t>>(last_var);
            if (wildcard_var.has_suffix_star_wildcard()) {
                logtype_query += enum_to_underlying_type(WildcardType::ZeroOrMoreChars);
            }
        }
    }
    logtype_query.append(m_query, constant_begin_pos, m_end_pos - constant_begin_pos);
}

template <typename encoded_variable_t>
bool CompositeWildcardToken<encoded_variable_t>::generate_next_interpretation() {
    for (auto& v : m_variables) {
        if (std::holds_alternative<WildcardToken<encoded_variable_t>>(v)) {
            auto& wildcard_var = std::get<WildcardToken<encoded_variable_t>>(v);
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
 * To turn a CompositeWildcardToken into ExactVariableTokens and WildcardTokens, we use the
 * following algorithm.
 *
 * Glossary:
 * - "token" - either an ExactVariableToken or a WildcardToken.
 * - "delimiter-wildcard" - a wildcard that is interpreted as matching delimiters.
 *
 * Overview:
 * - Each '*' at the edge of a token has one interpretation:
 *   1. matching a combination of non-delimiters and delimiters.
 * - Every other '*' has two interpretations:
 *   1. matching a combination of non-delimiters and delimiters, or
 *   2. only matching non-delimiters.
 * - Each '?' has two interpretations:
 *   1. matching a non-delimiter, or
 *   2. matching a delimiter.
 * - When tokenizing a CompositeWildcardToken, if none of its wildcards can match a delimiter, then
 *   the interpretation is simply the entire CompositeWildcardToken.
 * - However, if one of the wildcards can match a delimiter, then the CompositeWildcardToken splits
 *   into two tokens at the delimiter.
 * - Finally, if a WildcardToken is delimited by a '*'-delimiter-wildcard, then the '*' should be
 *   included in the WildcardToken (see the generalization in
 *   <docs>/dev-guide/design-parsing-wildcard-queries).
 *
 * Algorithm:
 * - To implement this algorithm, we need to search the CompositeWildcardToken for every substring
 *   bounded by wildcard-delimiters.
 * - For example, consider the CompositeWildcardToken "abc*def?ghi?123" and assume all wildcards are
 *   delimiter-wildcards:
 *   - The first token will be a WildcardToken, "abc*" (note that the '*' is included).
 *   - The second token will be a WildcardToken, "*def" (note that the '*' is included again).
 *   - The third substring will be static text, "ghi". Since this is neither a WildcardText nor an
 *     ExactVariableToken, it will be ignored.
 *   - The fourth token will be an ExactVariableToken, "123".
 * - If instead only the first '?' is interpreted as matching a delimiter, then the tokens will be
 *   ["*abc*def", "ghi?123"].
 *
 * NOTE: We could cache wildcard variables that we generate (using their bounds in the query as the
 * cache key) so that we don't end up regenerating them in other tokenizations. This isn't a
 * performance problem now, but could be an issue if we need to search the variable dictionary for
 * each generated WildcardToken.
 */
template <typename encoded_variable_t>
void CompositeWildcardToken<encoded_variable_t>::tokenize_into_wildcard_variable_tokens() {
    m_variables.clear();

    QueryWildcard const* last_wildcard = nullptr;
    bool wildcard_in_var = false;
    size_t var_begin_pos, var_end_pos;
    for (auto const& w : m_wildcards) {
        switch (w.get_current_interpretation()) {
            case WildcardInterpretation::NoDelimiters:
                wildcard_in_var = true;
                break;
            case WildcardInterpretation::ContainsDelimiters: {
                auto wildcard_pos = w.get_pos_in_query();
                if (wildcard_pos == m_begin_pos) {
                    last_wildcard = &w;
                    // Nothing to do yet since wildcard is at the beginning of the token
                    continue;
                }

                // Determine var_begin_pos
                if (nullptr == last_wildcard) {
                    var_begin_pos = m_begin_pos;
                } else {
                    if (WildcardType::ZeroOrMoreChars == last_wildcard->get_type()) {
                        // Include the wildcard in the token
                        var_begin_pos = last_wildcard->get_pos_in_query();
                        wildcard_in_var = true;
                    } else {
                        // Token starts after the wildcard
                        var_begin_pos = last_wildcard->get_pos_in_query() + 1;
                    }
                }

                // Determine var_end_pos
                if (WildcardType::ZeroOrMoreChars == w.get_type()) {
                    // Include the wildcard in the token
                    var_end_pos = wildcard_pos + 1;
                    wildcard_in_var = true;
                } else {
                    // Token ends before the wildcard
                    var_end_pos = wildcard_pos;
                }

                try_add_wildcard_variable(var_begin_pos, var_end_pos, wildcard_in_var);

                last_wildcard = &w;
                wildcard_in_var = false;
                break;
            }
            default:
                throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }
    }

    if (nullptr == last_wildcard) {
        // NOTE: Since the token contains a wildcard (this is the CompositeWildcardToken class),
        // there's no way this could be an ExactVariableToken
        m_variables.emplace_back(
                std::in_place_type<WildcardToken<encoded_variable_t>>,
                m_query,
                m_begin_pos,
                m_end_pos
        );
    } else if (last_wildcard->get_pos_in_query() < m_end_pos - 1) {
        if (WildcardType::ZeroOrMoreChars == last_wildcard->get_type()) {
            // Include the wildcard in the token
            var_begin_pos = last_wildcard->get_pos_in_query();
            wildcard_in_var = true;
        } else {
            var_begin_pos = last_wildcard->get_pos_in_query() + 1;
        }

        var_end_pos = m_end_pos;

        try_add_wildcard_variable(var_begin_pos, var_end_pos, wildcard_in_var);
    }
}

template <typename encoded_variable_t>
void CompositeWildcardToken<encoded_variable_t>::try_add_wildcard_variable(
        size_t begin_pos,
        size_t end_pos,
        bool wildcard_in_token
) {
    if (wildcard_in_token) {
        m_variables.emplace_back(
                std::in_place_type<WildcardToken<encoded_variable_t>>,
                m_query,
                begin_pos,
                end_pos
        );
    } else {
        string_view var(m_query.cbegin() + begin_pos, end_pos - begin_pos);
        if (ir::is_var(var)) {
            m_variables.emplace_back(
                    std::in_place_type<ExactVariableToken<encoded_variable_t>>,
                    m_query,
                    begin_pos,
                    end_pos
            );
        }
    }
}

// Explicitly declare specializations to avoid having to validate that the template parameters are
// supported
template class ffi::search::CompositeWildcardToken<ir::eight_byte_encoded_variable_t>;
template class ffi::search::CompositeWildcardToken<ir::four_byte_encoded_variable_t>;
}  // namespace clp::ffi::search
