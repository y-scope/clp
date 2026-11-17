#ifndef CLP_S_SEARCH_AST_CONSTANTPROP_HPP
#define CLP_S_SEARCH_AST_CONSTANTPROP_HPP

#include <memory>

#include <clp_s/search/ast/Expression.hpp>
#include <clp_s/search/ast/Transformation.hpp>

namespace clp_s::search::ast {
/**
 * Simplifies And/Or expressions in place by propagating constant operands (see `EmptyExpr`):
 * - An And with a false operand becomes false; its true operands are dropped.
 * - An Or with a true operand becomes true; its false operands are dropped.
 * - An And or Or left with no operands becomes true or false respectively.
 */
class ConstantProp : public Transformation {
public:
    // Methods implementing Transformation
    auto run(std::shared_ptr<Expression>& expr) -> std::shared_ptr<Expression> override;

private:
    // Static methods
    /**
     * Propagates constants through an expression tree.
     * @param cur
     * @return `cur` with constants propagated, or the constant it reduced to.
     */
    static auto propagate_constants(std::shared_ptr<Expression> cur) -> std::shared_ptr<Expression>;
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_AST_CONSTANTPROP_HPP
