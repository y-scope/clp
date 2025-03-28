#ifndef CLP_S_SEARCH_KQL_KQL_HPP
#define CLP_S_SEARCH_KQL_KQL_HPP

#include <istream>

#include "../ast/Expression.hpp"

namespace clp_s::search::kql {
/**
 * Generate a search AST from a Kibana expression in an input stream
 * @param in input stream containing a Kibana expression followed by EOF
 * @return a search AST on success, nullptr otherwise
 */
std::shared_ptr<clp_s::search::ast::Expression> parse_kql_expression(std::istream& in);
}  // namespace clp_s::search::kql

#endif  // CLP_S_SEARCH_KQL_KQL_HPP
