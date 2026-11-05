#include "OrExpr.hpp"

#include <memory>
#include <ostream>

#include <clp_s/search/ast/Expression.hpp>

namespace clp_s::search::ast {
OrExpr::OrExpr(bool inverted, Expression* parent) : Expression(inverted, parent) {}

auto OrExpr::print() const -> void {
    auto& os = get_print_stream();
    if (is_inverted()) {
        os << "!";
    }

    os << "OrExpr(";
    for (auto it = op_begin(); it != op_end();) {
        (*it)->print();
        it++;
        if (it != op_end()) {
            os << ", ";
        }
    }
    os << ")";

    if (get_parent() == nullptr) {
        os << '\n';
    }
    os << std::flush;
}

auto OrExpr::copy() const -> std::shared_ptr<Expression> {
    auto new_expr = std::shared_ptr<Expression>(new OrExpr(*this));
    for (auto it = new_expr->op_begin(); it != new_expr->op_end(); it++) {
        auto expr = std::static_pointer_cast<Expression>(*it);
        expr->copy_replace(new_expr.get(), it);
    }
    return new_expr;
}

auto OrExpr::create(bool inverted, Expression* parent) -> std::shared_ptr<Expression> {
    return std::shared_ptr<Expression>(static_cast<Expression*>(new OrExpr(inverted, parent)));
}

auto OrExpr::create(
        std::shared_ptr<Expression>& op1,
        std::shared_ptr<Expression>& op2,
        bool inverted,
        Expression* parent
) -> std::shared_ptr<Expression> {
    std::shared_ptr<Expression> expr(static_cast<Expression*>(new OrExpr(inverted, parent)));
    op1->copy_append(expr.get());
    op2->copy_append(expr.get());
    return expr;
}
}  // namespace clp_s::search::ast
