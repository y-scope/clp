#ifndef CLP_S_SEARCH_STRING_STORAGE_UTILS_HPP
#define CLP_S_SEARCH_STRING_STORAGE_UTILS_HPP

#include <clp_s/SchemaTree.hpp>
#include <clp_s/search/ast/Literal.hpp>

namespace clp_s::search {
/**
 * Determines whether a literal's string interpretation can never equal a string value stored as
 * `node_type`, using the clp-s storage invariant that a `ClpString` value always contains a space
 * and a `VarString` value never does.
 *
 * @param literal
 * @param node_type
 * @return Whether `literal` can never equal a value stored as `node_type`. Always false for any
 * other node type, and for a literal with no string interpretation, since the comparison is then
 * undefined rather than a defined non-match.
 */
[[nodiscard]] inline auto
can_never_equal_stored_string(ast::Literal const& literal, NodeType node_type) -> bool {
    switch (node_type) {
        case NodeType::ClpString:
            return ast::StringSpaceClass::ContainsNoSpace == literal.get_string_space_class();
        case NodeType::VarString:
            return ast::StringSpaceClass::ContainsSpace == literal.get_string_space_class();
        default:
            return false;
    }
}
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_STRING_STORAGE_UTILS_HPP
