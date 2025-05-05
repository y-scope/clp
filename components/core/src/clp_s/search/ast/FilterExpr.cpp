#include "FilterExpr.hpp"

#include <memory>
#include <ostream>
#include <string>

#include "ColumnDescriptor.hpp"
#include "Expression.hpp"
#include "FilterOperation.hpp"
#include "Literal.hpp"

namespace clp_s::search::ast {
auto FilterExpr::create(
        std::shared_ptr<ColumnDescriptor>& column,
        FilterOperation op,
        bool inverted,
        Expression* parent
) -> std::shared_ptr<Expression> {
    return std::shared_ptr<Expression>{
            static_cast<Expression*>(new FilterExpr{column->copy(), op, inverted, parent})
    };
}

auto FilterExpr::create(
        std::shared_ptr<ColumnDescriptor>& column,
        FilterOperation op,
        std::shared_ptr<Literal>& operand,
        bool inverted,
        Expression* parent
) -> std::shared_ptr<Expression> {
    std::shared_ptr<Expression> expr{
            static_cast<Expression*>(new FilterExpr{column->copy(), op, inverted, parent})
    };
    expr->add_operand(operand);
    return expr;
}

auto FilterExpr::op_type_str(FilterOperation op) -> std::string {
    switch (op) {
        case FilterOperation::EXISTS:
            return "EXISTS";
        case FilterOperation::NEXISTS:
            return "NEXISTS";
        case FilterOperation::EQ:
            return "EQ";
        case FilterOperation::NEQ:
            return "NEQ";
        case FilterOperation::LT:
            return "LT";
        case FilterOperation::GT:
            return "GT";
        case FilterOperation::LTE:
            return "LTE";
        case FilterOperation::GTE:
            return "GTE";
        default:
            return "UNKNOWN";
    }
}

auto FilterExpr::print() const -> void {
    auto& os = get_print_stream();
    if (is_inverted()) {
        os << "!";
    }

    os << "FilterExpr(";
    os << op_type_str(m_op);
    for (auto it = op_begin(); it != op_end(); it++) {
        os << ", ";
        (*it)->print();
    }
    os << ")";

    if (get_parent() == nullptr) {
        os << "\n";
    }
    os << std::flush;
}

auto FilterExpr::copy() const -> std::shared_ptr<Expression> {
    // Only deep copy column descriptors
    auto new_filter = std::shared_ptr<Expression>{static_cast<Expression*>(new FilterExpr{*this})};
    for (auto it = new_filter->op_begin(); it != new_filter->op_end(); it++) {
        if (auto descriptor = std::dynamic_pointer_cast<ColumnDescriptor>(*it)) {
            *it = descriptor->copy();
        }
    }
    return new_filter;
}

auto FilterExpr::get_operand() const -> std::shared_ptr<Literal> {
    auto it = op_begin();
    it++;
    if (it == op_end()) {
        return nullptr;
    }
    return std::static_pointer_cast<Literal>(*it);
}

FilterExpr::FilterExpr(
        std::shared_ptr<ColumnDescriptor> const& column,
        FilterOperation op,
        bool inverted,
        Expression* parent
)
        : Expression{inverted, parent},
          m_op{op} {
    add_operand(std::static_pointer_cast<Literal>(column));
}
}  // namespace clp_s::search::ast
