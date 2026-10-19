#ifndef CLP_S_SEARCH_AST_EMPTYEXPR_HPP
#define CLP_S_SEARCH_AST_EMPTYEXPR_HPP

#include <memory>
#include <optional>

#include <clp_s/search/ast/Expression.hpp>

namespace clp_s::search::ast {
/**
 * An expression with no sub-expressions that evaluates to a constant: false (the empty set), or
 * true when inverted. Useful for constant propagation and eliminating expressions.
 */
class EmptyExpr : public Expression {
public:
    // Factory methods
    /**
     * @param parent The parent this expression is attached to, if any.
     * @return The newly created expression, evaluating to false.
     */
    [[nodiscard]] static auto create(Expression* parent = nullptr) -> std::shared_ptr<Expression>;

    /**
     * @param value The constant the expression evaluates to.
     * @param parent The parent this expression is attached to, if any.
     * @return The newly created expression.
     */
    [[nodiscard]] static auto create_constant(bool value, Expression* parent = nullptr)
            -> std::shared_ptr<Expression>;

    // Static methods
    /**
     * @param expr
     * @return The constant `expr` evaluates to.
     * @return std::nullopt if `expr` is not an `EmptyExpr`.
     */
    [[nodiscard]] static auto as_constant(Expression const& expr) -> std::optional<bool>;

    // Delete copy assignment operator
    auto operator=(EmptyExpr const&) -> EmptyExpr& = delete;

    // Delete move constructor and assignment operator
    EmptyExpr(EmptyExpr&&) = delete;
    auto operator=(EmptyExpr&&) -> EmptyExpr& = delete;

    // Destructor
    ~EmptyExpr() override = default;

    // Methods implementing Expression
    // An `EmptyExpr` has no operands, so all of them are trivially expressions.
    [[nodiscard]] auto has_only_expression_operands() -> bool override { return true; }

    [[nodiscard]] auto copy() const -> std::shared_ptr<Expression> override;

    // Methods implementing Value
    auto print() const -> void override;

private:
    // Constructors
    explicit EmptyExpr(Expression* parent);

    EmptyExpr(EmptyExpr const&) = default;
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_AST_EMPTYEXPR_HPP
