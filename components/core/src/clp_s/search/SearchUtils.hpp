#ifndef CLP_S_SEARCH_SEARCHUTILS_HPP
#define CLP_S_SEARCH_SEARCHUTILS_HPP

#include <string>
#include <vector>

#include "Expression.hpp"
#include "Literal.hpp"

namespace clp_s::search {
/**
 * Splice a child expression into a parent expression at a given location
 * @param parent
 * @param child
 * @param location
 */
void splice_into(
        std::shared_ptr<Expression> const& parent,
        std::shared_ptr<Expression> const& child,
        OpList::iterator location
);

/**
 * Casts a double to an int64_t, rounding up or down depending on the filter operation
 * @param in
 * @param op
 * @param out
 * @return false if under FilterOperation::EQ the cast double is not equal to int64_t out, true
 * otherwise
 */
bool double_as_int(double in, FilterOperation op, int64_t& out);

/**
 * Converts a KQL string column descriptor delimited by '.' into a list of tokens. The
 * descriptor is tokenized and unescaped per the escaping rules for KQL columns.
 * @param descriptor
 * @param tokens
 * @param descriptor_namespace
 * @return true if the descriptor was tokenized successfully, false otherwise
 */
[[nodiscard]] auto tokenize_column_descriptor(
        std::string const& descriptor,
        std::vector<std::string>& tokens,
        std::string& descriptor_namespace
) -> bool;

/**
 * Unescapes a KQL value string according to the escaping rules for KQL value strings and
 * converts it into a valid CLP search string.
 *
 * Specifically this means that the string is unescaped, but the escape sequences '\\', '\*',
 * and '\?' are preserved so that the resulting string can be interpreted correctly by CLP
 * search.
 *
 * @param value
 * @param unescaped
 * @return true if the value was unescaped successfully, false otherwise.
 */
[[nodiscard]] auto unescape_kql_value(std::string const& value, std::string& unescaped) -> bool;
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_SEARCHUTILS_HPP
