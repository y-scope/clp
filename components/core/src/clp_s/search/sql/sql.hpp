#ifndef CLP_S_SEARCH_SQL_SQL_HPP
#define CLP_S_SEARCH_SQL_SQL_HPP

#include <istream>
#include <memory>

#include "../Expression.hpp"

namespace clp_s::search::sql {
/**
 * Parses an SQL expression from the given stream to generate a search AST.
 * @param in Input stream containing an SQL expression followed by EOF
 * @return a search AST on success, nullptr otherwise
 */
[[nodiscard]] auto parse_sql_expression(std::istream& in) -> std::shared_ptr<Expression>;
}  // namespace clp_s::search::sql

#endif  // CLP_S_SEARCH_SQL_SQL_HPP
