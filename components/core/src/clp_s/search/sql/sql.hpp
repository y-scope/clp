#ifndef CLP_S_SEARCH_SQL_SQL_HPP
#define CLP_S_SEARCH_SQL_SQL_HPP

#include <istream>

#include "../Expression.hpp"

namespace clp_s::search::sql {
/**
 * Generate a search AST from an SQL expression in an input stream
 * @param in input stream containing an SQL expression followed by EOF
 * @return a search AST on success, nullptr otherwise
 */
std::shared_ptr<Expression> parse_sql_expression(std::istream& in);
}  // namespace clp_s::search::sql

#endif  // CLP_S_SEARCH_SQL_SQL_HPP
