#include "NarrowTypes.hpp"

#include <array>
#include <memory>

#include "ColumnDescriptor.hpp"
#include "ConstantProp.hpp"
#include "EmptyExpr.hpp"
#include "FilterExpr.hpp"
#include "Literal.hpp"

namespace clp_s::search::ast {
std::shared_ptr<Expression> NarrowTypes::run(std::shared_ptr<Expression>& expr) {
    expr = narrow(expr);

    ConstantProp constant_prop;
    return constant_prop.run(expr);
}

std::shared_ptr<Expression> NarrowTypes::narrow(std::shared_ptr<Expression> cur) {
    if (cur->has_only_expression_operands()) {
        for (auto it = cur->op_begin(); it != cur->op_end(); it++) {
            auto child = std::static_pointer_cast<Expression>(*it);
            auto new_child = narrow(child);
            if (new_child != child) {
                new_child->copy_replace(cur.get(), it);
            }
        }
    } else if (auto filter = std::dynamic_pointer_cast<FilterExpr>(cur)) {
        // TODO: will have to change if we start supporting multi column expressions
        auto column = filter->get_column();
        auto op = filter->get_operation();

        if (op == FilterOperation::EXISTS || op == FilterOperation::NEXISTS) {
            return cur;
        }

        auto literal = filter->get_operand();

        if (false == literal->as_any(op)) {
            constexpr std::array cNarrowableLiteralTypes{
                    LiteralType::IntegerT,
                    LiteralType::FloatT,
                    LiteralType::BooleanT,
                    LiteralType::ArrayT,
                    LiteralType::NullT,
                    LiteralType::TimestampT,
                    LiteralType::StringT
            };
            for (auto const type : cNarrowableLiteralTypes) {
                if (literal->can_match_op(type, op)) {
                    continue;
                }
                column->remove_matching_type(type);
            }
        }

        if (false == column->matches_any(cAllTypes)) {
            return EmptyExpr::create();
        }
    }
    return cur;
}
}  // namespace clp_s::search::ast
