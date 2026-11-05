#ifndef CLP_S_SEARCH_ANDEXPR_HPP
#define CLP_S_SEARCH_ANDEXPR_HPP

#include <memory>

#include <clp_s/search/ast/Expression.hpp>

namespace clp_s::search::ast {
/**
 * Class representing a logical And operation across all
 * children in its OpList. Can have arbitrarily many children.
 */
class AndExpr : public Expression {
public:
    /**
     * Create an empty And expression which can optionally be inverted and attached to a parent.
     * Children can be added via mutators inherited from Expression.
     * @param inverted expression is inverted when true
     * @param parent parent this expression is attached to
     * @return Newly created Or expression
     */
    [[nodiscard]] static auto create(bool inverted = false, Expression* parent = nullptr)
            -> std::shared_ptr<Expression>;

    /**
     * Create an And expression with two children
     * @param op1 the first child operand
     * @param op2 the second child operand
     * @param inverted expression is inverted when true
     * @param parent parent this expression is attached to
     * @return Newly created Or expression
     */
    [[nodiscard]] static auto create(
            std::shared_ptr<Expression>& op1,
            std::shared_ptr<Expression>& op2,
            bool inverted = false,
            Expression* parent = nullptr
    ) -> std::shared_ptr<Expression>;

    // Delete copy assignment operator
    auto operator=(AndExpr const&) -> AndExpr& = delete;

    // Delete move constructor and assignment operator
    AndExpr(AndExpr&&) = delete;
    auto operator=(AndExpr&&) -> AndExpr& = delete;

    // Destructor
    ~AndExpr() override = default;

    // Methods inherited from Value
    auto print() const -> void override;

    // Methods inherited from Expression
    /**
     * And expressions only have other expressions as children by construction
     */
    [[nodiscard]] auto has_only_expression_operands() -> bool override { return true; }

    /**
     * Deep copy
     * @return A deep copy of this expression
     */
    [[nodiscard]] auto copy() const -> std::shared_ptr<Expression> override;

private:
    // Constructors
    explicit AndExpr(bool inverted = false, Expression* parent = nullptr);

    AndExpr(AndExpr const&) = default;
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_ANDEXPR_HPP
