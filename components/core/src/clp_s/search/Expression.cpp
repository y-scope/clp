#include "Expression.hpp"

namespace clp_s::search {
Expression::Expression(bool inverted, Expression* parent) {
    m_inverted = inverted;
    m_parent = parent;
}

Expression::Expression(Expression const& expr) {
    m_parent = nullptr;
    m_inverted = expr.m_inverted;
    m_operands = expr.m_operands;
}

void Expression::add_operand(std::shared_ptr<Expression> const& operand) {
    m_operands.push_back(std::static_pointer_cast<Value>(operand));
    operand->set_parent(this);
}

void Expression::add_operand(std::shared_ptr<Literal> const& operand) {
    m_operands.push_back(std::static_pointer_cast<Value>(operand));
}

void Expression::copy_append(Expression* parent) const {
    auto new_expr = this->copy();
    new_expr->set_parent(parent);
    parent->add_operand(new_expr);
}

void Expression::copy_replace(Expression* parent, OpList::iterator it) const {
    auto new_expr = this->copy();
    new_expr->set_parent(parent);
    *it = std::static_pointer_cast<Value>(new_expr);
}
}  // namespace clp_s::search
