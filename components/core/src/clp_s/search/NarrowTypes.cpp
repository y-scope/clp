#include "NarrowTypes.hpp"

#include <algorithm>

#include "ConstantProp.hpp"
#include "EmptyExpr.hpp"
#include "FilterExpr.hpp"
#include "Literal.hpp"
#include "OrExpr.hpp"
#include "OrOfAndForm.hpp"

namespace clp_s::search {
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
        std::string tmpstring;
        int64_t tmpint;
        double tmpdouble;
        bool tmpbool;
        bool narrowed_clp_string = false;
        bool narrowed_var_string = false;
        bool matches_var_and_clp_string = column->matches_type(LiteralType::ClpStringT)
                                          && column->matches_type(LiteralType::VarStringT);

        if (false == literal->as_any(op)) {
            if (false == literal->as_clp_string(tmpstring, op)) {
                narrowed_clp_string = true;
                column->remove_matching_type(LiteralType::ClpStringT);
            }
            if (false == literal->as_var_string(tmpstring, op)) {
                narrowed_var_string = true;
                column->remove_matching_type(LiteralType::VarStringT);
            }
            if (false == literal->as_int(tmpint, op)) {
                column->remove_matching_type(LiteralType::IntegerT);
            }
            if (false == literal->as_float(tmpdouble, op)) {
                column->remove_matching_type(LiteralType::FloatT);
            }
            if (false == literal->as_bool(tmpbool, op)) {
                column->remove_matching_type(LiteralType::BooleanT);
            }
            if (false == literal->as_array(tmpstring, op)) {
                column->remove_matching_type(LiteralType::ArrayT);
            }
            if (false == literal->as_null(op)) {
                column->remove_matching_type(LiteralType::NullT);
            }
            if (false == literal->as_epoch_date()) {
                column->remove_matching_type(LiteralType::EpochDateT);
            }
        }

        if (false == column->matches_any(cAllTypes)) {
            return EmptyExpr::create();
        }

        /**
         * Fix for bug y-scope/clp#254.
         * If:
         * - a filtering operation that can match a clp string and a var string is narrowed to just
         *   one of those underlying types
         * - AND the filter is performing a not equals operation
         * then we need to replace the filter with an OR across:
         * - the filter expression
         * - and an EXISTS expression for the other underlying type.
         *
         * This is because if we are trying to determine if some value is not equal to a specific
         * variable string, we can guarantee that every clp string is not equal to that specific
         * variable string (because that variable string must not contain a space, but any clp
         * string must). The reverse is true for checking if some value matches a specific clp
         * string.
         *
         * Really the issue is that we have multiple string types. Technically similar logic applies
         * for the date string column types, but we don't allow string matches against them so we
         * don't have to worry about supporting that here.
         */
        if (((filter->is_inverted() && FilterOperation::EQ == op)
             || (false == filter->is_inverted() && FilterOperation::NEQ == op))
            && matches_var_and_clp_string && narrowed_clp_string != narrowed_var_string)
        {
            auto exists_column_type
                    = narrowed_clp_string ? LiteralType::ClpStringT : LiteralType::VarStringT;
            auto exists_column = filter->get_column()->copy();
            exists_column->set_matching_type(exists_column_type);

            // This is an optimization to avoid creating multiple equivalent expressions in this
            // subtree of the AST. If the user has specified a long list of not equals expressions
            // on the same column (as is common when someone tries to filter out, e.g., a long list
            // of UUIDs) then we only need to transform one filter expression within the parent
            // OR/AND into an OR with the necessary EXISTS condition. Splitting each filter into an
            // OR could result in a much larger AST than necessary and slow down Schema Matching.
            auto it = std::find_if(
                    m_local_exists_descriptors.begin(),
                    m_local_exists_descriptors.end(),
                    [exists_column](auto const& rhs) -> bool { return *exists_column == *rhs; }
            );
            if (m_local_exists_descriptors.end() == it) {
                m_local_exists_descriptors.push_back(exists_column);
                auto exists_expr = FilterExpr::create(exists_column, FilterOperation::EXISTS);
                auto filter_as_expr_type = std::static_pointer_cast<Expression>(filter);
                return OrExpr::create(filter_as_expr_type, exists_expr);
            }
        }
    }
    return cur;
}
}  // namespace clp_s::search
