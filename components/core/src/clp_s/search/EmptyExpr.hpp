#ifndef CLP_S_SEARCH_EMPTYEXPR_HPP
#define CLP_S_SEARCH_EMPTYEXPR_HPP

#include "Expression.hpp"

namespace clp_s::search {
/**
 * Class representing the empty set/false. Useful
 * for constant propagation and eliminating expressions.
 */
class EmptyExpr : public Expression {
public:
    /**
     * Create an Empty expression which can optionally be attached to a parent
     * @param parent parent this expression is attached to
     * @return newly created Empty expression
     */
    static std::shared_ptr<Expression> create(Expression* parent = nullptr);

    // Methods inherited from Value
    void print() override;

    // Methods inherited from Expression
    // EmptyExpr never has any operands, so we arbitrarily say that all operands are Expression
    bool has_only_expression_operands() override { return true; }

    std::shared_ptr<Expression> copy() const override;

private:
    // Constructor
    explicit EmptyExpr(Expression* parent = nullptr);

    EmptyExpr(EmptyExpr const&);
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_EMPTYEXPR_HPP
