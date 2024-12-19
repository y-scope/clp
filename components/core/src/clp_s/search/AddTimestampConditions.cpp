#include "AddTimestampConditions.hpp"

#include "AndExpr.hpp"
#include "ColumnDescriptor.hpp"
#include "DateLiteral.hpp"
#include "EmptyExpr.hpp"
#include "FilterExpr.hpp"

namespace clp_s::search {
std::shared_ptr<Expression> AddTimestampConditions::run(std::shared_ptr<Expression>& expr) {
    if (false == m_begin_ts.has_value() && false == m_end_ts.has_value()) {
        return expr;
    }

    // If no timestamp column was passed then the timestamp filters do not make sense, and the query
    // shouldn't match any results.
    if (false == m_timestamp_column.has_value()) {
        return EmptyExpr::create();
    }

    auto timestamp_column
            = ColumnDescriptor::create_from_escaped_tokens(m_timestamp_column.value());

    auto and_expr = AndExpr::create();
    if (m_begin_ts.has_value()) {
        auto date_literal = DateLiteral::create_from_int(m_begin_ts.value());
        and_expr->add_operand(
                FilterExpr::create(timestamp_column, FilterOperation::GTE, date_literal)
        );
    }
    if (m_end_ts.has_value()) {
        auto date_literal = DateLiteral::create_from_int(m_end_ts.value());
        and_expr->add_operand(
                FilterExpr::create(timestamp_column, FilterOperation::LTE, date_literal)
        );
    }
    and_expr->add_operand(expr);

    return and_expr;
}
}  // namespace clp_s::search
