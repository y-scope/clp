#ifndef CLP_S_SEARCH_EMPTYEXPR_HPP
#define CLP_S_SEARCH_EMPTYEXPR_HPP

#include <memory>

#include <clp_s/search/ast/Expression.hpp>

namespace clp_s::search::ast {
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
    [[nodiscard]] static auto create(Expression* parent = nullptr) -> std::shared_ptr<Expression>;

    // Methods inherited from Value
    auto print() const -> void override;

    // Methods inherited from Expression
    // EmptyExpr never has any operands, so we arbitrarily say that all operands are Expression
    [[nodiscard]] auto has_only_expression_operands() -> bool override { return true; }

    [[nodiscard]] auto copy() const -> std::shared_ptr<Expression> override;

private:
    // Constructor
    explicit EmptyExpr(Expression* parent = nullptr);

    EmptyExpr(EmptyExpr const&);
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_EMPTYEXPR_HPP
