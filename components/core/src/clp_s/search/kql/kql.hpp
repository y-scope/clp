#ifndef CLP_S_SEARCH_KQL_KQL_HPP
#define CLP_S_SEARCH_KQL_KQL_HPP

#include <istream>
#include <memory>
#include <string_view>

#include <clp_s/search/ast/Expression.hpp>
#include <clp_s/search/ast/Value.hpp>

namespace clp_s::search::kql {
/**
 * Generate a search AST from a Kibana expression in an input stream.
 * @param in Input stream containing a Kibana expression followed by EOF.
 * @return A search AST on success, nullptr otherwise.
 */
auto parse_kql_expression(std::istream& in) -> std::shared_ptr<clp_s::search::ast::Expression>;

/**
 * Parse a projection column string, supporting both function calls and plain columns.
 * @param column_str The projection column string (e.g., "shape(message)" or "message").
 * @return A FunctionCall or ColumnDescriptor on success, nullptr otherwise.
 */
auto parse_projection_column(std::string_view column_str)
        -> std::shared_ptr<clp_s::search::ast::Value>;
}  // namespace clp_s::search::kql

#endif  // CLP_S_SEARCH_KQL_KQL_HPP
