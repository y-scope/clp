#include "Subquery.hpp"

// Project headers
#include "QueryWildcard.hpp"

using std::string;
using std::variant;
using std::vector;

namespace ffi::search {
    template <typename encoded_variable_t>
    Subquery<encoded_variable_t>::Subquery (string logtype_query,
                                            Subquery::QueryVariables variables)
            : m_logtype_query(std::move(logtype_query)), m_logtype_query_contains_wildcards(false),
            m_query_vars(variables)
    {
        // Determine if the query contains variables
        bool is_escaped = false;
        for (const auto c : m_logtype_query) {
            if (is_escaped) {
                is_escaped = false;
            } else if ('\\' == c) {
                is_escaped = true;
            } else if (enum_to_underlying_type(WildcardType::ZeroOrMoreChars) == c
                       || enum_to_underlying_type(WildcardType::AnyChar) == c)
            {
                m_logtype_query_contains_wildcards = true;
                break;
            }
        }
    }

    // Explicitly declare specializations to avoid having to validate that the
    // template parameters are supported
    template class Subquery<eight_byte_encoded_variable_t>;
    template class Subquery<four_byte_encoded_variable_t>;
}
