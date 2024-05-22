#include "FilterExpr.hpp"

namespace clp_s::search {
FilterExpr::FilterExpr(
        std::shared_ptr<ColumnDescriptor> const& column,
        FilterOperation op,
        bool inverted,
        Expression* parent
)
        : Expression(inverted, parent) {
    m_op = op;
    add_operand(std::static_pointer_cast<Literal>(column));
}

FilterExpr::FilterExpr(FilterExpr const& expr) : Expression(expr) {
    m_op = expr.m_op;
}

std::string FilterExpr::op_type_str(FilterOperation op) {
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

void FilterExpr::print() {
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
        os << std::endl;
    } else {
        os << std::flush;
    }
}

std::shared_ptr<Expression> FilterExpr::create(
        std::shared_ptr<ColumnDescriptor>& column,
        FilterOperation op,
        bool inverted,
        Expression* parent
) {
    return std::shared_ptr<Expression>(
            static_cast<Expression*>(new FilterExpr(column->copy(), op, inverted, parent))
    );
}

std::shared_ptr<Expression> FilterExpr::create(
        std::shared_ptr<ColumnDescriptor>& column,
        FilterOperation op,
        std::shared_ptr<Literal>& operand,
        bool inverted,
        Expression* parent
) {
    std::shared_ptr<Expression> expr(
            static_cast<Expression*>(new FilterExpr(column->copy(), op, inverted, parent))
    );
    expr->add_operand(operand);
    return expr;
}

std::shared_ptr<Expression> FilterExpr::copy() const {
    // Only deep copy column descriptors
    auto new_filter = std::shared_ptr<Expression>(static_cast<Expression*>(new FilterExpr(*this)));
    for (auto it = new_filter->op_begin(); it != new_filter->op_end(); it++) {
        if (auto descriptor = std::dynamic_pointer_cast<ColumnDescriptor>(*it)) {
            *it = descriptor->copy();
        }
    }
    return new_filter;
}

std::shared_ptr<Literal> FilterExpr::get_operand() {
    auto it = op_begin();
    it++;
    if (it == op_end()) {
        return nullptr;
    } else {
        return std::static_pointer_cast<Literal>(*it);
    }
}
}  // namespace clp_s::search
