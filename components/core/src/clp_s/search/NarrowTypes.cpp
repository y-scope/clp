#include "NarrowTypes.hpp"

#include "ConstantProp.hpp"
#include "EmptyExpr.hpp"
#include "FilterExpr.hpp"
#include "Literal.hpp"

namespace clp_s::search {
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
        std::string tmpstring;
        int64_t tmpint;
        double tmpdouble;
        bool tmpbool;

        if (false == literal->as_any(op)) {
            if (false == literal->as_clp_string(tmpstring, op)) {
                column->remove_matching_type(LiteralType::ClpStringT);
            }
            if (false == literal->as_var_string(tmpstring, op)) {
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
            if (false == literal->as_float_date()) {
                column->remove_matching_type(LiteralType::EpochDateT);
            }
        }

        if (false == column->matches_any(cAllTypes)) {
            return EmptyExpr::create();
        }
    }
    return cur;
}
}  // namespace clp_s::search
