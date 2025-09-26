#ifndef GLT_FFI_SEARCH_COMPOSITEWILDCARDTOKEN_HPP
#define GLT_FFI_SEARCH_COMPOSITEWILDCARDTOKEN_HPP

#include <string_view>
#include <variant>
#include <vector>

#include "ExactVariableToken.hpp"
#include "QueryToken.hpp"
#include "QueryWildcard.hpp"
#include "WildcardToken.hpp"

namespace glt::ffi::search {
/**
 * A token delimited by delimiters and non-wildcards. Note that the original query string is stored
 * by reference, so it must remain valid while the token exists.
 * <br>
 * For instance, in the query "var:*abc?def*", "*abc?def*" would be a CompositeWildcardToken. This
 * is different from a WildcardToken which can be delimited by wildcards. For instance, "*abc" could
 * be a WildcardToken, where it's delimited by '?' (on the right).
 * <br>
 * By interpreting wildcards (as matching delimiters/non-delimiters) within a CompositeWildcardToken
 * and then tokenizing the CompositeWildcardToken's value, we can generate ExactVariableTokens and
 * WildcardTokens. That's why this is called a CompositeWildcardToken.
 * @tparam encoded_variable_t Type for encoded variable values
 */
template <typename encoded_variable_t>
class CompositeWildcardToken : public QueryToken {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        [[nodiscard]] char const* what() const noexcept override {
            return "ffi::search::CompositeWildcardToken operation failed";
        }
    };

    // Constructors
    CompositeWildcardToken(std::string_view query, size_t begin_pos, size_t end_pos);

    // Methods
    /**
     * Populates the logtype query and @p variable_tokens based on the current interpretation of
     * wildcards and WildcardTokens
     * @param logtype_query
     * @param variable_tokens
     */
    void add_to_query(
            std::string& logtype_query,
            std::vector<std::variant<
                    ExactVariableToken<encoded_variable_t>,
                    WildcardToken<encoded_variable_t>>>& variable_tokens
    ) const;

    /**
     * Generates the next interpretation of this token
     * @return true if there was another interpretation to advance to
     * @return false if we overflowed to the first interpretation
     */
    bool generate_next_interpretation();

private:
    // Methods
    /**
     * Tokenizes this CompositeWildcardToken into ExactVariableTokens and WildcardTokens based on
     * the current interpretation of wildcards
     */
    void tokenize_into_wildcard_variable_tokens();
    /**
     * Adds the token given by the string bounds to the vector of variables, iff the token contains
     * a wildcard (and so could be a variable) or the token is indeed a variable.
     * @param begin_pos
     * @param end_pos
     * @param wildcard_in_token
     */
    void try_add_wildcard_variable(size_t begin_pos, size_t end_pos, bool wildcard_in_token);

    // Variables
    std::vector<QueryWildcard> m_wildcards;
    std::vector<
            std::variant<ExactVariableToken<encoded_variable_t>, WildcardToken<encoded_variable_t>>>
            m_variables;
};
}  // namespace glt::ffi::search

#endif  // GLT_FFI_SEARCH_COMPOSITEWILDCARDTOKEN_HPP
