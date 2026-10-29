#include "SetTimestampLiteralPrecision.hpp"

#include <memory>
#include <vector>

#include "Expression.hpp"
#include "TimestampLiteral.hpp"

namespace clp_s::search::ast {
auto SetTimestampLiteralPrecision::run(std::shared_ptr<Expression>& expr)
        -> std::shared_ptr<Expression> {
    std::vector<std::shared_ptr<Expression>> work_list{expr};
    while (false == work_list.empty()) {
        auto const sub_expr{work_list.back()};
        work_list.pop_back();

        if (sub_expr->has_only_expression_operands()) {
            for (auto const& op : sub_expr->get_op_list()) {
                work_list.emplace_back(std::static_pointer_cast<Expression>(op));
            }
            continue;
        }

        for (auto const& op : sub_expr->get_op_list()) {
            if (auto timestamp_literal{std::dynamic_pointer_cast<TimestampLiteral>(op)};
                nullptr != timestamp_literal)
            {
                timestamp_literal->set_default_precision(m_precision);
            }
        }
    }
    return expr;
}
}  // namespace clp_s::search::ast
