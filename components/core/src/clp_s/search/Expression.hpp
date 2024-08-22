#ifndef CLP_S_SEARCH_EXPRESSION_HPP
#define CLP_S_SEARCH_EXPRESSION_HPP

#include <list>
#include <memory>

#include "Literal.hpp"
#include "Value.hpp"

namespace clp_s::search {
using OpList = std::list<std::shared_ptr<Value>>;

/**
 * Top level class for all logical expressions which represent filters
 * on columns.
 *
 * Key subclasses are AndExpr, OrExpr, and FilterExpr
 */
class Expression : public Value {
public:
    /**
     * True if this expression is inverted
     * @return Whether the expression is inverted
     */
    bool is_inverted() const { return m_inverted; }

    /**
     * Flip whether the expression is inverted
     */
    void invert() { m_inverted = !m_inverted; }

    /**
     * @return The number of operands that this expression has
     */
    unsigned get_num_operands() override { return m_operands.size(); }

    /**
     * Get iterators to this Expression's OpList
     * @return Iterators to the beggining/end of the OpList
     */
    OpList::iterator op_begin() { return m_operands.begin(); }

    OpList::iterator op_end() { return m_operands.end(); }

    /**
     * @return A reference to the underlying OpList. Useful in cases where certain children
     * need to be deleted, or multiple children need to be spliced in.
     */
    OpList& get_op_list() { return m_operands; }

    /**
     * Add an operand to the end of the OpList. When the operand is an
     * Expression its parent is set to this Expression.
     * @param operand the operand to append to the OpList
     */
    void add_operand(std::shared_ptr<Expression> const& operand);

    void add_operand(std::shared_ptr<Literal> const& operand);

    /**
     * @return The parent for this Expression. Can be nullptr if this is the top level.
     */
    Expression* get_parent() { return m_parent; }

    /**
     * Set the parent for this Expression
     * @param parent this Expression's new parent
     */
    void set_parent(Expression* parent) { m_parent = parent; }

    /**
     * Deep copy
     * @return A deep copy of this expression
     */
    virtual std::shared_ptr<Expression> copy() const = 0;

    /**
     * Deep copy this expression and append it into *parent*'s OpList.
     * Also sets the parent for copy to parent.
     * @param parent the parent to copy into
     */
    void copy_append(Expression* parent) const;

    /**
     * Deep copy this expression and replace a specific operand in the
     * *parent*'s OpList.
     * @param parent the parent to copy into
     * @param it an iterator into the parent's OpList representing the operand that will get
     * replaced
     */
    void copy_replace(Expression* parent, OpList::iterator it) const;

    /**
     * Whether this Expression's operands are all Expression
     * @return true if this Expression's operands are all Expression
     */
    virtual bool has_only_expression_operands() = 0;

    // Methods inherited from Value
    void print() override = 0;

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
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_EXPRESSION_HPP
