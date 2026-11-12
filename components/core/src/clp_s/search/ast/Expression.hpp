#ifndef CLP_S_SEARCH_EXPRESSION_HPP
#define CLP_S_SEARCH_EXPRESSION_HPP

#include <cstddef>
#include <list>
#include <memory>
#include <optional>

#include "Literal.hpp"
#include "Value.hpp"

namespace clp_s::search::ast {
using OpList = std::list<std::shared_ptr<Value>>;

/**
 * Top level class for all logical expressions which represent filters
 * on columns.
 *
 * Key subclasses are AndExpr, OrExpr, and FilterExpr
 */
class Expression : public Value {
public:
    // Delete copy assignment operator
    auto operator=(Expression const&) -> Expression& = delete;

    // Delete move constructor and assignment operator
    Expression(Expression&&) = delete;
    auto operator=(Expression&&) -> Expression& = delete;

    // Destructor
    ~Expression() override = default;

    /**
     * True if this expression is inverted
     * @return Whether the expression is inverted
     */
    [[nodiscard]] auto is_inverted() const -> bool { return m_inverted; }

    /**
     * Flip whether the expression is inverted
     */
    auto invert() -> void { m_inverted = !m_inverted; }

    /**
     * @return The number of operands that this expression has
     */
    [[nodiscard]] auto get_num_operands() const -> size_t override { return m_operands.size(); }

    /**
     * Gets iterators to this `Expression`'s OpList.
     * @return Iterators to the beginning/end of the OpList
     */
    [[nodiscard]] auto op_begin() -> OpList::iterator { return m_operands.begin(); }

    [[nodiscard]] auto op_end() -> OpList::iterator { return m_operands.end(); }

    [[nodiscard]] auto op_begin() const -> OpList::const_iterator { return m_operands.cbegin(); }

    [[nodiscard]] auto op_end() const -> OpList::const_iterator { return m_operands.cend(); }

    /**
     * @return A reference to the underlying OpList. Useful in cases where certain children
     * need to be deleted, or multiple children need to be spliced in.
     */
    auto get_op_list() -> OpList& { return m_operands; }

    /**
     * Add an operand to the end of the OpList. When the operand is an
     * Expression its parent is set to this Expression.
     * @param operand the operand to append to the OpList
     */
    auto add_operand(std::shared_ptr<Expression> const& operand) -> void;

    auto add_operand(std::shared_ptr<Literal> const& operand) -> void;

    /**
     * @return The parent for this Expression. Can be nullptr if this is the top level.
     */
    [[nodiscard]] auto get_parent() const -> Expression* { return m_parent; }

    /**
     * Set the parent for this Expression
     * @param parent this Expression's new parent
     */
    auto set_parent(Expression* parent) -> void { m_parent = parent; }

    /**
     * Deep copy
     * @return A deep copy of this expression
     */
    [[nodiscard]] virtual auto copy() const -> std::shared_ptr<Expression> = 0;

    /**
     * Deep copy this expression and append it into *parent*'s OpList.
     * Also sets the parent for copy to parent.
     * @param parent the parent to copy into
     */
    auto copy_append(Expression* parent) const -> void;

    /**
     * Deep copy this expression and replace a specific operand in the
     * *parent*'s OpList.
     * @param parent the parent to copy into
     * @param it an iterator into the parent's OpList representing the operand that will get
     * replaced
     */
    auto copy_replace(Expression* parent, OpList::iterator it) const -> void;

    /**
     * Whether this Expression's operands are all Expression
     * @return true if this Expression's operands are all Expression
     */
    [[nodiscard]] virtual auto has_only_expression_operands() -> bool = 0;

    // Methods inherited from Value
    auto print() const -> void override = 0;

protected:
    /**
     * All expressions can be inverted, have a parent (nullptr for top level),
     * and have 0 or more operands
     */
    bool m_inverted;
    Expression* m_parent;
    std::list<std::shared_ptr<Value>> m_operands;

    // Copy Semantic is create shallow copy with parent pointing to null
    Expression(Expression const&);

    explicit Expression(bool inverted, Expression* parent = nullptr);
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_EXPRESSION_HPP
