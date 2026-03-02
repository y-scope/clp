#include "AddTimestampConditions.hpp"

#include "../Defs.hpp"
#include "../Utils.hpp"
#include "ast/AndExpr.hpp"
#include "ast/ColumnDescriptor.hpp"
#include "ast/EmptyExpr.hpp"
#include "ast/Expression.hpp"
#include "ast/FilterExpr.hpp"
#include "ast/FilterOperation.hpp"
#include "ast/TimestampLiteral.hpp"

using clp_s::search::ast::AndExpr;
using clp_s::search::ast::ColumnDescriptor;
using clp_s::search::ast::EmptyExpr;
using clp_s::search::ast::Expression;
using clp_s::search::ast::FilterExpr;
using clp_s::search::ast::FilterOperation;
using clp_s::search::ast::TimestampLiteral;

namespace clp_s::search {
namespace {
constexpr epochtime_t cNanosecondsInMillisecond{1000LL * 1000LL};
}  // namespace

std::shared_ptr<Expression> AddTimestampConditions::run(std::shared_ptr<Expression>& expr) {
    if (false == m_begin_ts.has_value() && false == m_end_ts.has_value()) {
        return expr;
    }

    // If no timestamp column was passed then the timestamp filters do not make sense, and the query
    // shouldn't match any results.
    if (false == m_timestamp_column.has_value()) {
        return EmptyExpr::create();
    }

    auto timestamp_column = ColumnDescriptor::create_from_escaped_tokens(
            m_timestamp_column.value().first,
            m_timestamp_column.value().second
    );

    auto and_expr = AndExpr::create();
    if (m_begin_ts.has_value()) {
        auto timestamp_literal
                = TimestampLiteral::create(m_begin_ts.value() * cNanosecondsInMillisecond);
        and_expr->add_operand(
                FilterExpr::create(timestamp_column, FilterOperation::GTE, timestamp_literal)
        );
    }
    if (m_end_ts.has_value()) {
        auto timestamp_literal
                = TimestampLiteral::create(m_end_ts.value() * cNanosecondsInMillisecond);
        and_expr->add_operand(
                FilterExpr::create(timestamp_column, FilterOperation::LTE, timestamp_literal)
        );
    }
    and_expr->add_operand(expr);

    return and_expr;
}
}  // namespace clp_s::search
