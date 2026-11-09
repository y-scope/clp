#include "ConstantProp.hpp"

#include <memory>
#include <vector>

#include <clp_s/search/ast/AndExpr.hpp>
#include <clp_s/search/ast/EmptyExpr.hpp>
#include <clp_s/search/ast/Expression.hpp>
#include <clp_s/search/ast/OrExpr.hpp>

namespace clp_s::search::ast {
auto ConstantProp::run(std::shared_ptr<Expression>& expr) -> std::shared_ptr<Expression> {
    return propagate_empty(expr);
}

auto ConstantProp::propagate_empty(std::shared_ptr<Expression> cur) -> std::shared_ptr<Expression> {
    if (std::dynamic_pointer_cast<OrExpr>(cur)) {
        std::vector<OpList::iterator> deleted;
        for (auto it = cur->op_begin(); it != cur->op_end(); it++) {
            auto new_child = propagate_empty(std::static_pointer_cast<Expression>(*it));
            if (std::dynamic_pointer_cast<EmptyExpr>(new_child)) {
                deleted.push_back(it);
            }
        }

        if (deleted.size() == cur->get_op_list().size()) {
            return EmptyExpr::create(cur->get_parent());
        }

        for (auto const& it : deleted) {
            cur->get_op_list().erase(it);
        }
    } else if (std::dynamic_pointer_cast<AndExpr>(cur)) {
        for (auto it = cur->op_begin(); it != cur->op_end(); it++) {
            auto new_child = propagate_empty(std::static_pointer_cast<Expression>(*it));
            if (std::dynamic_pointer_cast<EmptyExpr>(new_child)) {
                new_child->set_parent(cur->get_parent());
                return new_child;
            }
        }
    }

    return cur;
}
}  // namespace clp_s::search::ast
