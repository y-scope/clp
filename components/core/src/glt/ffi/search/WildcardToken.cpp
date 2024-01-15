#include "WildcardToken.hpp"

#include <string_view>

#include <string_utils/string_utils.hpp>

#include "../../ir/types.hpp"
#include "../../type_utils.hpp"
#include "../encoding_methods.hpp"
#include "QueryWildcard.hpp"

using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::four_byte_encoded_variable_t;
using clp::ir::VariablePlaceholder;
using std::string;
using std::string_view;

namespace clp::ffi::search {
// Local function prototypes
/**
 * @tparam encoded_variable_t Type of the encoded variable
 * @param token
 * @return Whether the given string could be an encoded float variable
 */
template <typename encoded_variable_t>
static bool could_be_float_var(string_view token);
/**
 * @tparam encoded_variable_t Type of the encoded variable
 * @param token
 * @return Whether the given string could be an encoded integer variable
 */
template <typename encoded_variable_t>
static bool could_be_int_var(string_view token);
/**
 * @param query
 * @param begin_pos
 * @param end_pos
 * @return Whether the given string could be static text in a log message
 */
static bool could_be_static_text(string_view query, size_t begin_pos, size_t end_pos);

template <typename encoded_variable_t>
static bool could_be_float_var(string_view token) {
    size_t num_decimals = 0;
    size_t num_negative_signs = 0;
    size_t num_digits = 0;
    for (auto c : token) {
        if ('.' == c) {
            ++num_decimals;
            if (num_decimals > 1) {
                // Contains multiple decimal points
                return false;
            }
        } else if ('-' == c) {
            ++num_negative_signs;
            if (num_negative_signs > 1) {
                // Contains multiple negative signs
                return false;
            }
        } else if ('0' <= c && c <= '9') {
            ++num_digits;
            constexpr size_t cMaxDigitsInRepresentableFloatVar
                    = std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>
                              ? cMaxDigitsInRepresentableFourByteFloatVar
                              : cMaxDigitsInRepresentableEightByteFloatVar;
            if (num_digits > cMaxDigitsInRepresentableFloatVar) {
                // More digits than is representable
                return false;
            }
        } else if ('*' != c && '?' != c) {
            // Not a wildcard
            return false;
        }
    }
    return true;
}

template <typename encoded_variable_t>
static bool could_be_int_var(string_view token) {
    size_t num_negative_signs = 0;
    size_t num_digits = 0;
    for (auto c : token) {
        if ('-' == c) {
            ++num_negative_signs;
            if (num_negative_signs > 1) {
                // Contains multiple negative signs
                return false;
            }
        } else if ('0' <= c && c <= '9') {
            ++num_digits;
            // ceil(log10(INT32_MAX))
            constexpr size_t cMaxDigitsInRepresentableFourByteIntVar = 10;
            // ceil(log10(INT64_MAX))
            constexpr size_t cMaxDigitsInRepresentableEightByteIntVar = 19;
            constexpr size_t cMaxDigitsInRepresentableIntVar
                    = std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>
                              ? cMaxDigitsInRepresentableFourByteIntVar
                              : cMaxDigitsInRepresentableEightByteIntVar;
            if (num_digits > cMaxDigitsInRepresentableIntVar) {
                // More digits than is representable
                return false;
            }
        } else if ('*' != c && '?' != c) {
            // Not a wildcard
            return false;
        }
    }
    return true;
}

/**
 * To check if the token could be static text, formally, we need to check if the token matches the
 * complement of all variable schemas ORed together (~((schema1)|(schema2)|...). Another way of
 * looking at this is if the token contains anything which indicates it's definitely a variable,
 * then it can't be static text.
 */
static bool could_be_static_text(string_view query, size_t begin_pos, size_t end_pos) {
    bool is_escaped = false;
    bool contains_alphabet = false;
    for (size_t i = begin_pos; i < end_pos; ++i) {
        auto c = query[i];
        if (is_escaped) {
            is_escaped = false;
        } else if ('\\' == c) {
            is_escaped = true;
        } else if (string_utils::is_decimal_digit(c)) {
            return false;
        } else if (string_utils::is_alphabet(c)) {
            contains_alphabet = true;
        }
    }

    if (begin_pos > 0 && '=' == query[begin_pos - 1]) {
        if ('?' == query[begin_pos] && contains_alphabet) {
            // "=?...<alphabet>..." must be a variable since
            // 1. '?' would only be included in the variable token if it was treated as a
            //    non-delimiter, and
            // 2. an '=' followed by non-delimiters and an alphabet is definitely a variable.
            return false;
        }
    }

    return true;
}

template <typename encoded_variable_t>
WildcardToken<encoded_variable_t>::WildcardToken(
        string_view query,
        size_t begin_pos,
        size_t end_pos
)
        : QueryToken(query, begin_pos, end_pos),
          m_has_prefix_star_wildcard('*' == query[begin_pos]),
          m_has_suffix_star_wildcard('*' == query[end_pos - 1]) {
    auto token = string_view(query.cbegin() + begin_pos, end_pos - begin_pos);
    if (could_be_int_var<encoded_variable_t>(token)) {
        m_possible_variable_types.push_back(TokenType::IntegerVariable);
    }
    if (could_be_float_var<encoded_variable_t>(token)) {
        m_possible_variable_types.push_back(TokenType::FloatVariable);
    }
    if (could_be_static_text(query, begin_pos, end_pos)) {
        m_possible_variable_types.push_back(TokenType::StaticText);
    }
    // Value must contain a wildcard and a non-delimiter, so it can be a
    // dictionary variable
    m_possible_variable_types.push_back(TokenType::DictionaryVariable);

    m_current_interpretation_idx = 0;
}

template <typename encoded_variable_t>
bool WildcardToken<encoded_variable_t>::add_to_logtype_query(string& logtype_query) const {
    // Recall from CompositeWildcardToken::add_to_query: We need to handle '*' carefully when adding
    // to the logtype query since we may have a token like "a1*b2" with interpretation ["a1*",
    // "*b2"], i.e., the first token's suffix '*' is the second token's prefix '*'. So we only add
    // the current token's prefix '*' below and ignore any suffix '*' since they will be captured by
    // the next token.
    auto current_interpretation = m_possible_variable_types[m_current_interpretation_idx];
    if (TokenType::StaticText == current_interpretation) {
        if (m_has_suffix_star_wildcard) {
            // Ignore the suffix '*'
            logtype_query.append(m_query, m_begin_pos, (m_end_pos - 1) - m_begin_pos);
        } else {
            logtype_query.append(m_query, m_begin_pos, m_end_pos - m_begin_pos);
        }
        return false;
    } else {
        if (m_has_prefix_star_wildcard) {
            logtype_query += enum_to_underlying_type(WildcardType::ZeroOrMoreChars);
        }
        switch (current_interpretation) {
            case TokenType::DictionaryVariable:
                logtype_query += enum_to_underlying_type(VariablePlaceholder::Dictionary);
                break;
            case TokenType::FloatVariable:
                logtype_query += enum_to_underlying_type(VariablePlaceholder::Float);
                break;
            case TokenType::IntegerVariable:
                logtype_query += enum_to_underlying_type(VariablePlaceholder::Integer);
                break;
            default:
                throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }
        return true;
    }
}

template <typename encoded_variable_t>
bool WildcardToken<encoded_variable_t>::next_interpretation() {
    ++m_current_interpretation_idx;
    if (m_current_interpretation_idx < m_possible_variable_types.size()) {
        return true;
    } else {
        m_current_interpretation_idx = 0;
        return false;
    }
}

// Explicitly declare specializations to avoid having to validate that the template parameters are
// supported
template class WildcardToken<eight_byte_encoded_variable_t>;
template class WildcardToken<four_byte_encoded_variable_t>;
}  // namespace clp::ffi::search
