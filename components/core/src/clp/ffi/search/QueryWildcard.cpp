#include "QueryWildcard.hpp"

#include "../../type_utils.hpp"

namespace clp::ffi::search {
QueryWildcard::QueryWildcard(char wildcard, size_t pos_in_query, bool is_boundary_wildcard) {
    if (enum_to_underlying_type(WildcardType::AnyChar) != wildcard
        && enum_to_underlying_type(WildcardType::ZeroOrMoreChars) != wildcard)
    {
        throw QueryWildcardOperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    m_type = static_cast<WildcardType>(wildcard);
    m_pos_in_query = pos_in_query;

    if (is_boundary_wildcard && WildcardType::ZeroOrMoreChars == m_type) {
        // We don't need to consider the "NoDelimiters" case for '*' at the ends of the token since
        // it wouldn't change the interpretation of the token. See the README for more details.
        m_possible_interpretations.emplace_back(WildcardInterpretation::ContainsDelimiters);
    } else {
        m_possible_interpretations.emplace_back(WildcardInterpretation::ContainsDelimiters);
        m_possible_interpretations.emplace_back(WildcardInterpretation::NoDelimiters);
    }
    m_current_interpretation_idx = 0;
}

bool QueryWildcard::next_interpretation() {
    ++m_current_interpretation_idx;
    if (m_current_interpretation_idx < m_possible_interpretations.size()) {
        return true;
    } else {
        m_current_interpretation_idx = 0;
        return false;
    }
}
}  // namespace clp::ffi::search
