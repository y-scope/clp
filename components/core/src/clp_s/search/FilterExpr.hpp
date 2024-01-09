#ifndef CLP_S_SEARCH_FILTEREXPR_HPP
#define CLP_S_SEARCH_FILTEREXPR_HPP

#include <string>

#include "ColumnDescriptor.hpp"
#include "Expression.hpp"
#include "FilterOperation.hpp"
#include "Literal.hpp"

namespace clp_s::search {
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
     * @return FilterOperation this Filter performs
     */
    FilterOperation get_operation() { return m_op; }

    /**
     * @return The Column this Filter acts on
     */
    std::shared_ptr<ColumnDescriptor> get_column() {
        return std::static_pointer_cast<ColumnDescriptor>(*op_begin());
    }

    /**
     * @return This Filter's Literal or nullptr if there is no Literal
     */
    std::shared_ptr<Literal> get_operand();

    /**
     * Create a Filter expression with a Column and FilterOperation but no Literal
     * Literal can be added later using mutators provided by the Expression parent class
     * @param column the Column this Filter acts on
     * @param op the Operation this Filter uses to Filter the Column
     * @param inverted expression is inverted when true
     * @param parent parent this expression is attached to
     * @return Newly created Or expression
     */
    static std::shared_ptr<Expression> create(
            std::shared_ptr<ColumnDescriptor>& column,
            FilterOperation op,
            bool inverted = false,
            Expression* parent = nullptr
    );

    /**
     * Create a Filter expression with a Column, FilterOperation and Literal
     * @param column the Column this Filter acts on
     * @param op the Operation this Filter uses to Filter the Column
     * @param inverted expression is inverted when true
     * @param parent parent this expression is attached to
     * @return newly created Or expression
     */
    static std::shared_ptr<Expression> create(
            std::shared_ptr<ColumnDescriptor>& column,
            FilterOperation op,
            std::shared_ptr<Literal>& operand,
            bool inverted = false,
            Expression* parent = nullptr
    );

    /**
     * Helper function to turn FilterOperation into string for printing
     * @param op the operation we want to convert to string
     * @return a string representing the operation
     */
    static std::string op_type_str(FilterOperation op);

    // Methods inherited from Value
    void print() override;

    // Methods inherited from Expression
    bool has_only_expression_operands() override { return false; }

    std::shared_ptr<Expression> copy() const override;

private:
    FilterOperation m_op;

    // Constructor
    FilterExpr(
            std::shared_ptr<ColumnDescriptor> const& column,
            FilterOperation op,
            bool inverted = false,
            Expression* parent = nullptr
    );

    FilterExpr(FilterExpr const&);
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_FILTEREXPR_HPP
