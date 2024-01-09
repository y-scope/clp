#include "OrOfAndForm.hpp"

#include <vector>

#include "SearchUtils.hpp"

namespace clp_s::search {
std::shared_ptr<Expression> OrOfAndForm::run(std::shared_ptr<Expression>& expr) {
    auto parent = expr->get_parent();
    while (expr->get_num_operands() == 1 && expr->has_only_expression_operands()) {
        bool invert = expr->is_inverted();
        expr = std::static_pointer_cast<Expression>(*expr->op_begin());
        expr->set_parent(parent);
        if (invert) {
            expr->invert();
        }
    }

    if (expr->is_inverted()) {
        de_morgan(expr);
    }

    // only need to further simplify and/or expressions
    if (false == expr->has_only_expression_operands()) {
        return expr;
    }

    return simplify(expr);
}

void OrOfAndForm::de_morgan(std::shared_ptr<Expression>& expr) {
    std::shared_ptr<Expression> new_expr;

    if (std::dynamic_pointer_cast<AndExpr>(expr)) {
        new_expr = OrExpr::create(!expr->is_inverted(), expr->get_parent());
    } else if (std::dynamic_pointer_cast<OrExpr>(expr)) {
        new_expr = AndExpr::create(!expr->is_inverted(), expr->get_parent());
    } else {
        // DeMorgan's doesn't apply; no modification required
        return;
    }

    new_expr->get_op_list().splice(new_expr->op_end(), expr->get_op_list());
    for (auto it = new_expr->op_begin(); it != new_expr->op_end(); it++) {
        auto sub_expr = std::static_pointer_cast<Expression>(*it);
        sub_expr->set_parent(new_expr.get());
        sub_expr->invert();
    }

    expr = new_expr;
}

std::shared_ptr<Expression> OrOfAndForm::simplify(std::shared_ptr<Expression> const& expr) {
    for (auto it = expr->op_begin(); it != expr->op_end(); it++) {
        auto sub_expr = std::static_pointer_cast<Expression>(*it);
        if (sub_expr->is_inverted()) {
            // DeMorgan's already makes checks that input is Or or And so don't
            // need to double check here
            de_morgan(sub_expr);
            *it = sub_expr;
        }

        while (sub_expr->get_num_operands() == 1 && sub_expr->has_only_expression_operands()) {
            bool invert = sub_expr->is_inverted();
            sub_expr = std::static_pointer_cast<Expression>(*sub_expr->op_begin());
            sub_expr->set_parent(expr.get());
            *it = sub_expr;
            if (invert) {
                sub_expr->invert();
            }
        }

        // Only need to simplify Or/And subexpr
        if (sub_expr->has_only_expression_operands()) {
            *it = simplify(sub_expr);
        }
    }

    if (std::dynamic_pointer_cast<OrExpr>(expr)) {
        return simplify_or(expr);
    } else if (std::dynamic_pointer_cast<AndExpr>(expr)) {
        return simplify_and(expr);
    } else {
        // currently and/or are the only form of expressions we need to simplify
        return expr;
    }
}

std::shared_ptr<Expression> OrOfAndForm::simplify_or(std::shared_ptr<Expression> const& expr) {
    std::vector<OpList::iterator> deleted;

    for (auto it = expr->op_begin(); it != expr->op_end(); it++) {
        if (std::dynamic_pointer_cast<OrExpr>(*it)) {
            auto sub_expr = std::static_pointer_cast<Expression>(*it);
            deleted.push_back(it);
            splice_into(expr, sub_expr, expr->op_begin());
        }
    }

    for (auto const& it : deleted) {
        expr->get_op_list().erase(it);
    }

    return expr;
}

std::shared_ptr<Expression> OrOfAndForm::simplify_and(std::shared_ptr<Expression> const& expr) {
    std::vector<OpList::iterator> deleted;
    std::vector<OpList::iterator> deleted_or_expr;
    std::vector<std::shared_ptr<Expression>> or_expressions;

    for (auto it = expr->op_begin(); it != expr->op_end(); it++) {
        if (std::dynamic_pointer_cast<AndExpr>(*it)) {
            auto sub_expr = std::static_pointer_cast<Expression>(*it);
            deleted.push_back(it);
            splice_into(expr, sub_expr, expr->op_begin());
        } else if (std::dynamic_pointer_cast<OrExpr>(*it)) {
            deleted_or_expr.push_back(it);
        }
    }

    for (auto const& it : deleted) {
        expr->get_op_list().erase(it);
    }

    if (deleted_or_expr.empty()) {
        return expr;
    }

    for (auto const& it : deleted_or_expr) {
        or_expressions.push_back(std::static_pointer_cast<Expression>(*it));
        expr->get_op_list().erase(it);
    }

    auto new_or_expr = OrExpr::create(false, expr->get_parent());
    ExpressionList prefix;
    insert_all_combinations(
            new_or_expr,
            expr,
            or_expressions.begin(),
            or_expressions.end(),
            prefix
    );

    return new_or_expr;
}

void OrOfAndForm::insert_all_combinations(
        std::shared_ptr<Expression> const& new_or_expr,
        std::shared_ptr<Expression> const& base_and_expr,
        ExpressionVector::iterator cur,
        ExpressionVector::iterator end,
        ExpressionList& prefix
) {
    if (cur == end) {
        auto new_and_expr = base_and_expr->copy();
        for (auto const& it : prefix) {
            // these OrExpr are guaranteed to contain only FilterExpr/AndExpr
            if (std::dynamic_pointer_cast<AndExpr>(it)) {
                splice_into(new_and_expr, it->copy(), new_and_expr->op_end());
            } else {
                it->copy_append(new_and_expr.get());
            }
        }
        new_or_expr->add_operand(new_and_expr);
        return;
    }

    auto current_or = *cur;
    cur++;
    for (auto it = current_or->op_begin(); it != current_or->op_end(); it++) {
        prefix.push_back(std::static_pointer_cast<Expression>(*it));
        auto cur_copy = cur;
        cur_copy++;
        insert_all_combinations(new_or_expr, base_and_expr, cur, end, prefix);
        prefix.pop_back();
    }
}
}  // namespace clp_s::search
