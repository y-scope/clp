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

    // Find the name of the timestamp column
    // TODO: At the moment we only allow a single timestamp column at ingestion time, but the
    // timestamp dictionary is designed to store the ranges of several timestamp columns. We should
    // enforce a convention that the first entry in the timestamp dictionary corresponds to the
    // "authoratative" timestamp column for the dataset.
    std::shared_ptr<ColumnDescriptor> timestamp_column;
    for (auto it = m_timestamp_dict->tokenized_column_to_range_begin();
         it != m_timestamp_dict->tokenized_column_to_range_end();
         ++it)
    {
        timestamp_column = ColumnDescriptor::create(it->first);
        break;
    }

    // If there is no authoratative timestamp column then the timestamp filters specified by the
    // user do not make sense, and the query shouldn't match any results.
    if (nullptr == timestamp_column) {
        return EmptyExpr::create();
    }

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
