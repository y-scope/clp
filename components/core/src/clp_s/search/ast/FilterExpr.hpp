#ifndef CLP_S_SEARCH_FILTEREXPR_HPP
#define CLP_S_SEARCH_FILTEREXPR_HPP

#include <memory>
#include <string>

#include "ColumnDescriptor.hpp"
#include "Expression.hpp"
#include "FilterOperation.hpp"
#include "Literal.hpp"

namespace clp_s::search::ast {
/**
 * Class for simple filter conditions in the AST. Consists of a column,
 * a filtering operation, and usually a literal.
 *
 * Conventionally the OpList contains a ColumnExpr followed by some Literal. I.e. a FilterExpr
 * always has a ColumnExpr, but may not have a Literal.
 */
class FilterExpr : public Expression {
public:
    /**
     * Create a Filter expression with a Column and FilterOperation but no Literal
     * Literal can be added later using mutators provided by the Expression parent class
     * @param column the Column this Filter acts on
     * @param op the Operation this Filter uses to Filter the Column
     * @param inverted expression is inverted when true
     * @param parent parent this expression is attached to
     * @return Newly created Or expression
     */
    [[nodiscard]] static auto create(
            std::shared_ptr<ColumnDescriptor>& column,
            FilterOperation op,
            bool inverted = false,
            Expression* parent = nullptr
    ) -> std::shared_ptr<Expression>;

    /**
     * Create a Filter expression with a Column, FilterOperation and Literal
     * @param column the Column this Filter acts on
     * @param op the Operation this Filter uses to Filter the Column
     * @param inverted expression is inverted when true
     * @param parent parent this expression is attached to
     * @return newly created Or expression
     */
    [[nodiscard]] static auto create(
            std::shared_ptr<ColumnDescriptor>& column,
            FilterOperation op,
            std::shared_ptr<Literal>& operand,
            bool inverted = false,
            Expression* parent = nullptr
    ) -> std::shared_ptr<Expression>;

    /**
     * Helper function to turn FilterOperation into string for printing
     * @param op the operation we want to convert to string
     * @return a string representing the operation
     */
    [[nodiscard]] static auto op_type_str(FilterOperation op) -> std::string;

    // Delete copy assignment operator
    auto operator=(FilterExpr const&) -> FilterExpr& = delete;

    // Delete move constructor and assignment operator
    FilterExpr(FilterExpr&&) = delete;
    auto operator=(FilterExpr&&) -> FilterExpr& = delete;

    // Destructor
    ~FilterExpr() override = default;

    // Methods inherited from Value
    auto print() const -> void override;

    // Methods inherited from Expression
    [[nodiscard]] auto has_only_expression_operands() -> bool override { return false; }

    [[nodiscard]] auto copy() const -> std::shared_ptr<Expression> override;

    // Methods
    /**
     * @return The FilterOperation performed by this Filter.
     */
    [[nodiscard]] auto get_operation() const -> FilterOperation { return m_op; }

    /**
     * @return The ColumnDescriptor that this Filter acts on.
     */
    [[nodiscard]] auto get_column() -> std::shared_ptr<ColumnDescriptor> {
        return std::static_pointer_cast<ColumnDescriptor>(*op_begin());
    }

    /**
     * @return This Filter's Literal or nullptr if there is no Literal.
     */
    [[nodiscard]] auto get_operand() const -> std::shared_ptr<Literal>;

private:
    // Constructor
    FilterExpr(
            std::shared_ptr<ColumnDescriptor> const& column,
            FilterOperation op,
            bool inverted = false,
            Expression* parent = nullptr
    );

    // Default copy constructor
    FilterExpr(FilterExpr const&) = default;

    // Variables
    FilterOperation m_op;
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_FILTEREXPR_HPP
