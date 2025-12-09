#include "SetDateLiteralPrecision.hpp"

#include <memory>
#include <vector>

#include "DateLiteral.hpp"
#include "Expression.hpp"

namespace clp_s::search::ast {
auto SetDateLiteralPrecision::run(std::shared_ptr<Expression>& expr)
        -> std::shared_ptr<Expression> {
    std::vector<std::shared_ptr<Expression>> work_list{expr};
    while (false == work_list.empty()) {
        auto const expr{work_list.back()};
        work_list.pop_back();

        if (expr->has_only_expression_operands()) {
            for (auto const& op : expr->get_op_list()) {
                work_list.emplace_back(std::static_pointer_cast<Expression>(op));
            }
            continue;
        }

        for (auto const& op : expr->get_op_list()) {
            if (auto date_literal{std::dynamic_pointer_cast<DateLiteral>(op)};
                nullptr != date_literal)
            {
                date_literal->set_default_precision(m_precision);
            }
        }
    }
    return expr;
}
}  // namespace clp_s::search::ast
