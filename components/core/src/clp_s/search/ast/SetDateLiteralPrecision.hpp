#ifndef CLP_S_SEARCH_AST_SETDATELITERALPRECISION_HPP
#define CLP_S_SEARCH_AST_SETDATELITERALPRECISION_HPP

#include <memory>

#include "DateLiteral.hpp"
#include "Expression.hpp"
#include "Transformation.hpp"

namespace clp_s::search::ast {
/**
 * Transformation pass that sets the precision of all date literals in the expression AST to a
 * given precision.
 */
class SetDateLiteralPrecision : public Transformation {
public:
    // Constructors
    explicit SetDateLiteralPrecision(DateLiteral::Precision precision) : m_precision{precision} {}

    // Methods inherited from Transformation
    auto run(std::shared_ptr<Expression>& expr) -> std::shared_ptr<Expression> override;

private:
    // Members
    DateLiteral::Precision m_precision{DateLiteral::Precision::Nanoseconds};
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_AST_SETDATELITERALPRECISION_HPP
