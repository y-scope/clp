#include "NarrowTypes.hpp"

#include <algorithm>
#include <array>
#include <memory>

#include "ConstantProp.hpp"
#include "EmptyExpr.hpp"
#include "FilterExpr.hpp"
#include "Literal.hpp"
#include "OrExpr.hpp"
#include "OrOfAndForm.hpp"

namespace clp_s::search::ast {
std::shared_ptr<Expression> NarrowTypes::run(std::shared_ptr<Expression>& expr) {
    expr = narrow(expr);

    if (m_should_renormalize) {
        OrOfAndForm normalize;
        expr = normalize.run(expr);
    }

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
                m_should_renormalize = true;
            }
        }
        m_local_exists_descriptors.clear();
    } else if (auto filter = std::dynamic_pointer_cast<FilterExpr>(cur)) {
        // TODO: will have to change if we start supporting multi column expressions
        auto column = filter->get_column();
        auto op = filter->get_operation();

        if (op == FilterOperation::EXISTS || op == FilterOperation::NEXISTS) {
            return cur;
        }

        auto literal = filter->get_operand();
        bool narrowed_clp_string = false;
        bool narrowed_var_string = false;
        bool const matches_var_and_clp_string = column->matches_type(LiteralType::VarStringT)
                                                && column->matches_type(LiteralType::ClpStringT);

        if (false == literal->as_any(op)) {
            constexpr std::array cNarrowableLiteralTypes{
                    LiteralType::IntegerT,
                    LiteralType::FloatT,
                    LiteralType::ClpStringT,
                    LiteralType::VarStringT,
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
                narrowed_clp_string |= LiteralType::ClpStringT == type;
                narrowed_var_string |= LiteralType::VarStringT == type;
            }
        }

        if (false == column->matches_any(cAllTypes)) {
            return EmptyExpr::create();
        }

        if (auto const rewritten{rewrite_negated_string_equality(
                    filter,
                    matches_var_and_clp_string,
                    narrowed_clp_string,
                    narrowed_var_string
            )};
            nullptr != rewritten)
        {
            return rewritten;
        }
    }
    return cur;
}

auto NarrowTypes::rewrite_negated_string_equality(
        std::shared_ptr<FilterExpr> const& filter,
        bool matches_var_and_clp_string,
        bool narrowed_clp_string,
        bool narrowed_var_string
) -> std::shared_ptr<Expression> {
    auto const& column{filter->get_column()};
    auto const op{filter->get_operation()};
    bool const is_negated_equality{
            (filter->is_inverted() && FilterOperation::EQ == op)
            || (false == filter->is_inverted() && FilterOperation::NEQ == op)
    };
    if (false == is_negated_equality || false == matches_var_and_clp_string
        || narrowed_clp_string == narrowed_var_string || column->is_pure_wildcard()
        || column->is_shape_query())
    {
        return nullptr;
    }

    auto exists_column_type
            = narrowed_clp_string ? LiteralType::ClpStringT : LiteralType::VarStringT;
    auto exists_column = column->copy();
    exists_column->set_matching_type(exists_column_type);

    // Reuse a sibling's EXISTS expression when it pins the same column and type.
    //
    // TODO: Under an AND parent this rewrite is unsound: AND(OR(A, E), B) requires B to hold in
    // schemas where only E holds, but B's narrowed column doesn't resolve there. The correct
    // rewrite is OR(AND(A, B), E) at the AND level.
    auto it = std::find_if(
            m_local_exists_descriptors.begin(),
            m_local_exists_descriptors.end(),
            [exists_column](auto const& rhs) -> bool { return *exists_column == *rhs; }
    );
    if (m_local_exists_descriptors.end() != it) {
        return nullptr;
    }

    m_local_exists_descriptors.push_back(exists_column);
    auto exists_expr = FilterExpr::create(exists_column, FilterOperation::EXISTS);
    auto filter_as_expr_type = std::static_pointer_cast<Expression>(filter);
    return OrExpr::create(filter_as_expr_type, exists_expr);
}
}  // namespace clp_s::search::ast
