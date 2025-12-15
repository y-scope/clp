#ifndef CLP_S_SEARCH_AST_SETTIMESTAMPLITERALPRECISION_HPP
#define CLP_S_SEARCH_AST_SETTIMESTAMPLITERALPRECISION_HPP

#include <memory>

#include "Expression.hpp"
#include "TimestampLiteral.hpp"
#include "Transformation.hpp"

namespace clp_s::search::ast {
/**
 * Transformation pass that sets the precision of all timestamp literals in the expression AST to a
 * given precision.
 */
class SetTimestampLiteralPrecision : public Transformation {
public:
    explicit SetTimestampLiteralPrecision(TimestampLiteral::Precision precision)
            : m_precision{precision} {}

    /**
     * @param expr
     * @return A mutated version of `expr` where all `TimestampLiteral`s have precision set to
     * `m_precision`.
     */
    auto run(std::shared_ptr<Expression>& expr) -> std::shared_ptr<Expression> override;

private:
    TimestampLiteral::Precision m_precision{TimestampLiteral::Precision::Nanoseconds};
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_AST_SETTIMESTAMPLITERALPRECISION_HPP
