#include "ConvertToExists.hpp"

#include "ColumnDescriptor.hpp"
#include "ConstantProp.hpp"
#include "EmptyExpr.hpp"
#include "FilterExpr.hpp"
#include "Literal.hpp"
#include "OrExpr.hpp"
#include "OrOfAndForm.hpp"

namespace clp_s::search {
std::shared_ptr<Expression> ConvertToExists::run(std::shared_ptr<Expression>& expr) {
    expr = convert(expr);

    if (m_needs_standard_form) {
        OrOfAndForm pass;
        expr = pass.run(expr);
    }

    if (m_needs_constant_prop) {
        ConstantProp pass;
        expr = pass.run(expr);
    }

    return expr;
}

std::shared_ptr<Expression> ConvertToExists::convert(std::shared_ptr<Expression> cur) {
    if (cur->has_only_expression_operands()) {
        for (auto it = cur->op_begin(); it != cur->op_end(); it++) {
            auto child = std::static_pointer_cast<Expression>(*it);
            auto new_child = convert(child);
            if (new_child != child) {
                new_child->copy_replace(cur.get(), it);
            }
        }
    } else if (auto filter = std::dynamic_pointer_cast<FilterExpr>(cur)) {
        // TODO: will have to change if we start supporting multi column expressions
        auto column = filter->get_column();
        auto op = filter->get_operation();

        if (op == FilterOperation::EXISTS || op == FilterOperation::NEXISTS) {
            if (false == filter->is_inverted()) {
                return cur;
            }

            FilterOperation new_op = (op == FilterOperation::EXISTS) ? FilterOperation::NEXISTS
                                                                     : FilterOperation::EXISTS;
            auto new_col = column->copy();
            return FilterExpr::create(new_col, new_op);
        }

        auto literal = filter->get_operand();

        bool exists;
        if (filter->is_inverted()) {
            exists = op == FilterOperation::NEQ;
        } else {
            exists = op == FilterOperation::EQ;
        }

        if (literal->as_any(op)) {
            auto new_col = column->copy();
            if (exists) {
                return FilterExpr::create(new_col, FilterOperation::EXISTS);
            } else {
                return FilterExpr::create(new_col, FilterOperation::NEXISTS);
            }
        } else if (literal->as_null(op)) {
            auto new_col = column->copy();
            auto new_col_null = column->copy();
            if (exists) {
                m_needs_standard_form = true;
                new_col->remove_matching_types(
                        cAllTypes
                        & ~(LiteralType::ArrayT | LiteralType::ClpStringT | LiteralType::VarStringT)
                );
                new_col_null->remove_matching_types(cAllTypes & ~LiteralType::NullT);
                std::shared_ptr<Expression> non_null_filter;
                if (new_col->matches_any(cAllTypes)) {
                    non_null_filter = FilterExpr::create(new_col, FilterOperation::EQ);
                    non_null_filter->add_operand(literal);
                } else {
                    non_null_filter = EmptyExpr::create();
                    m_needs_constant_prop = true;
                }

                std::shared_ptr<Expression> null_filter;
                if (new_col_null->matches_any(cAllTypes)) {
                    null_filter = FilterExpr::create(new_col_null, FilterOperation::EXISTS);
                } else {
                    null_filter = EmptyExpr::create();
                    m_needs_constant_prop = true;
                }

                return OrExpr::create(null_filter, non_null_filter);
            } else {
                if (new_col->matches_type(LiteralType::NullT)) {
                    // != null supercedes all other types
                    new_col->set_matching_types(cAllTypes & ~LiteralType::NullT);
                    return FilterExpr::create(new_col, FilterOperation::EXISTS);
                } else {
                    new_col->remove_matching_type(LiteralType::NullT);
                    if (new_col->matches_any(cAllTypes)) {
                        return FilterExpr::create(new_col, FilterOperation::EXISTS);
                    } else {
                        m_needs_constant_prop = true;
                        return EmptyExpr::create();
                    }
                }
            }
        }
    }
    return cur;
}
}  // namespace clp_s::search
