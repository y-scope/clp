#ifndef CLP_S_SEARCH_OREXPR_HPP
#define CLP_S_SEARCH_OREXPR_HPP

#include <memory>

#include <clp_s/search/ast/Expression.hpp>

namespace clp_s::search::ast {
/**
 * Class representing a logical Or operation across all
 * children in its OpList. Can have arbitrarily many children.
 */
class OrExpr : public Expression {
public:
    /**
     * Create an empty Or expression which can optionally be inverted and attached to a parent
     * Children can be added via mutators inherited from Expression.
     * @param inverted expression is inverted when true
     * @param parent parent this expression is attached to
     * @return A newly created Or expression
     */
    [[nodiscard]] static auto create(bool inverted = false, Expression* parent = nullptr)
            -> std::shared_ptr<Expression>;

    /**
     * Create an Or expression with two children
     * @param op1 the first child operand
     * @param op2 the second child operand
     * @param inverted expression is inverted when true
     * @param parent parent this expression is attached to
     * @return A newly created Or expression
     */
    [[nodiscard]] static auto create(
            std::shared_ptr<Expression>& op1,
            std::shared_ptr<Expression>& op2,
            bool inverted = false,
            Expression* parent = nullptr
    ) -> std::shared_ptr<Expression>;

    // Delete copy assignment operator
    auto operator=(OrExpr const&) -> OrExpr& = delete;

    // Delete move constructor and assignment operator
    OrExpr(OrExpr&&) = delete;
    auto operator=(OrExpr&&) -> OrExpr& = delete;

    // Destructor
    ~OrExpr() override = default;

    // Methods inherited from Value
    auto print() const -> void override;

    // Methods inherited from Expression
    [[nodiscard]] auto has_only_expression_operands() -> bool override { return true; }

    [[nodiscard]] auto copy() const -> std::shared_ptr<Expression> override;

private:
    // Constructors
    explicit OrExpr(bool inverted = false, Expression* parent = nullptr);

    OrExpr(OrExpr const&) = default;
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_OREXPR_HPP
