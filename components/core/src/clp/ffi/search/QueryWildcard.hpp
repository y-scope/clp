#ifndef CLP_FFI_SEARCH_QUERYWILDCARD_HPP
#define CLP_FFI_SEARCH_QUERYWILDCARD_HPP

#include <vector>

#include "../../TraceableException.hpp"

namespace clp::ffi::search {
enum class WildcardType : char {
    AnyChar = '?',
    ZeroOrMoreChars = '*',
};

/**
 * Possible interpretations of what is matched by a wildcard in a query
 */
enum class WildcardInterpretation {
    // Matches anything except delimiters
    NoDelimiters = 0,
    // For '*', matches anything including delimiters
    // For '?', matches a delimiter
    ContainsDelimiters,
};

/**
 * Class representing a wildcard in a query
 */
class QueryWildcard {
public:
    // Types
    class QueryWildcardOperationFailed : public TraceableException {
    public:
        // Constructors
        QueryWildcardOperationFailed(
                ErrorCode error_code,
                char const* const filename,
                int line_number
        )
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        [[nodiscard]] char const* what() const noexcept override {
            return "ffi::search::QueryWildcard operation failed";
        }
    };

    // Constructors
    /**
     * Constructs a query wildcard
     * @param wildcard
     * @param pos_in_query
     * @param is_boundary_wildcard Whether this wildcard is at either end of the query token
     */
    QueryWildcard(char wildcard, size_t pos_in_query, bool is_boundary_wildcard);

    // Methods
    /**
     * Advances to the next interpretation of the query wildcard
     * @return true if there was another interpretation to advance to
     * @return false if we overflowed to the first interpretation
     */
    bool next_interpretation();

    [[nodiscard]] WildcardInterpretation get_current_interpretation() const {
        return m_possible_interpretations[m_current_interpretation_idx];
    }

    [[nodiscard]] size_t get_pos_in_query() const { return m_pos_in_query; }

    [[nodiscard]] WildcardType get_type() const { return m_type; }

private:
    WildcardType m_type;
    size_t m_pos_in_query;
    std::vector<WildcardInterpretation> m_possible_interpretations;
    size_t m_current_interpretation_idx;
};
}  // namespace clp::ffi::search

#endif  // CLP_FFI_SEARCH_QUERYWILDCARD_HPP
