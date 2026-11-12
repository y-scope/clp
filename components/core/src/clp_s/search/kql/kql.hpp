#ifndef CLP_S_SEARCH_KQL_KQL_HPP
#define CLP_S_SEARCH_KQL_KQL_HPP

#include <istream>
#include <memory>
#include <string_view>

#include <clp_s/search/ast/ColumnDescriptor.hpp>
#include <clp_s/search/ast/Expression.hpp>

namespace clp_s::search::kql {
/**
 * Generate a search AST from a Kibana expression in an input stream.
 * @param in Input stream containing a Kibana expression followed by EOF.
 * @return A search AST on success, nullptr otherwise.
 */
auto parse_kql_expression(std::istream& in) -> std::shared_ptr<clp_s::search::ast::Expression>;

/**
 * Parses a projection column string. The string is treated as if it were a double-quoted KQL
 * column name (e.g. `my key` names the key `my key`; `a\.b` names the key `a.b`), except when it
 * has the form `function(column)` (e.g. `shape(message)`): then the argument is written as a column
 * would be in a query, so names containing spaces or special characters must be double-quoted
 * (e.g. `shape("my key")`). A column whose own name looks like a function call can be projected by
 * escaping the parenthesis (`foo\(bar\)`).
 * @param column_str The projection column string.
 * @return A ColumnDescriptor on success, nullptr otherwise.
 */
auto parse_projection_column(std::string_view column_str)
        -> std::shared_ptr<clp_s::search::ast::ColumnDescriptor>;
}  // namespace clp_s::search::kql

#endif  // CLP_S_SEARCH_KQL_KQL_HPP
