#ifndef CLP_S_SEARCH_KQL_KQL_HPP
#define CLP_S_SEARCH_KQL_KQL_HPP

#include <istream>

#include "../ast/Expression.hpp"

namespace clp_s::search::kql {
/**
 * Generates a search AST from a Kibana Query Language (KQL) expression.
 * @param in An input stream containing a KQL expression followed by EOF.
 * @return A search AST on success or nullptr on failure.
 */
[[nodiscard]] auto parse_kql_expression(std::istream& in)
        -> std::shared_ptr<clp_s::search::ast::Expression>;
}  // namespace clp_s::search::kql

#endif  // CLP_S_SEARCH_KQL_KQL_HPP
