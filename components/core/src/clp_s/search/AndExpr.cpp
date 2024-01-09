#include "AndExpr.hpp"

#include <iostream>

namespace clp_s::search {
AndExpr::AndExpr(bool inverted, Expression* parent) : Expression(inverted, parent) {}

AndExpr::AndExpr(AndExpr const& expr) : Expression(expr) {}

void AndExpr::print() {
    auto& os = get_print_stream();
    if (is_inverted()) {
        os << "!";
    }

    os << "AndExpr(";
    for (auto it = op_begin(); it != op_end();) {
        (*it)->print();
        it++;
        if (it != op_end()) {
            os << ", ";
        }
    }
    os << ")";

    if (get_parent() == nullptr) {
        os << std::endl;
    } else {
        os << std::flush;
    }
}

std::shared_ptr<Expression> AndExpr::copy() const {
    auto new_expr = std::shared_ptr<Expression>(new AndExpr(*this));
    for (auto it = new_expr->op_begin(); it != new_expr->op_end(); it++) {
        auto expr = std::static_pointer_cast<Expression>(*it);
        expr->copy_replace(new_expr.get(), it);
    }
    return new_expr;
}

std::shared_ptr<Expression> AndExpr::create(bool inverted, Expression* parent) {
    return std::shared_ptr<Expression>(static_cast<Expression*>(new AndExpr(inverted, parent)));
}

std::shared_ptr<Expression> AndExpr::create(
        std::shared_ptr<Expression>& op1,
        std::shared_ptr<Expression>& op2,
        bool inverted,
        Expression* parent
) {
    std::shared_ptr<Expression> expr(static_cast<Expression*>(new AndExpr(inverted, parent)));
    op1->copy_append(expr.get());
    op2->copy_append(expr.get());
    return expr;
}
}  // namespace clp_s::search
