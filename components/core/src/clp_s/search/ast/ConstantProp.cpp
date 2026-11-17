#include "ConstantProp.hpp"

#include <memory>
#include <vector>

#include <clp_s/search/ast/AndExpr.hpp>
#include <clp_s/search/ast/EmptyExpr.hpp>
#include <clp_s/search/ast/Expression.hpp>
#include <clp_s/search/ast/OrExpr.hpp>

namespace clp_s::search::ast {
auto ConstantProp::run(std::shared_ptr<Expression>& expr) -> std::shared_ptr<Expression> {
    return propagate_constants(expr);
}

// And and Or are handled uniformly in terms of two values: the "absorbing" value, which decides the
// result as soon as one operand equals it (true for Or, false for And), and the "identity" value,
// which never affects the result and so can be dropped (the opposite). When the expression reduces
// to a constant, its own inversion is applied to that constant.
auto ConstantProp::propagate_constants(std::shared_ptr<Expression> cur)
        -> std::shared_ptr<Expression> {
    bool const is_or{nullptr != std::dynamic_pointer_cast<OrExpr>(cur)};
    if (false == is_or && nullptr == std::dynamic_pointer_cast<AndExpr>(cur)) {
        return cur;
    }
    bool const absorbing_value{is_or};
    bool const identity_value{false == absorbing_value};

    std::vector<OpList::iterator> identity_operands;
    for (auto it = cur->op_begin(); it != cur->op_end(); it++) {
        auto const constant{EmptyExpr::as_constant(
                *propagate_constants(std::static_pointer_cast<Expression>(*it))
        )};
        if (false == constant.has_value()) {
            continue;
        }
        if (absorbing_value == constant.value()) {
            return EmptyExpr::create_constant(
                    absorbing_value != cur->is_inverted(),
                    cur->get_parent()
            );
        }
        identity_operands.push_back(it);
    }

    if (identity_operands.size() == cur->get_op_list().size()) {
        return EmptyExpr::create_constant(identity_value != cur->is_inverted(), cur->get_parent());
    }
    for (auto const& it : identity_operands) {
        cur->get_op_list().erase(it);
    }
    return cur;
}
}  // namespace clp_s::search::ast
