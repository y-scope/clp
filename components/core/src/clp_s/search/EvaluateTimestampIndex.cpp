#include "EvaluateTimestampIndex.hpp"

#include <memory>

#include "ast/AndExpr.hpp"
#include "ast/Expression.hpp"
#include "ast/FilterExpr.hpp"
#include "ast/FilterOperation.hpp"
#include "ast/Integral.hpp"
#include "ast/Literal.hpp"
#include "ast/OrExpr.hpp"
#include "ast/TimestampLiteral.hpp"

using clp_s::search::ast::AndExpr;
using clp_s::search::ast::Expression;
using clp_s::search::ast::FilterExpr;
using clp_s::search::ast::FilterOperation;
using clp_s::search::ast::Integral;
using clp_s::search::ast::Integral64;
using clp_s::search::ast::literal_type_bitmask_t;
using clp_s::search::ast::OrExpr;
using clp_s::search::ast::TimestampLiteral;

namespace clp_s::search {
constexpr literal_type_bitmask_t cTimestampTypes
        = search::ast::cIntegralTypes | search::ast::LiteralType::TimestampT;

EvaluatedValue EvaluateTimestampIndex::run(std::shared_ptr<Expression> const& expr) {
    if (std::dynamic_pointer_cast<OrExpr>(expr)) {
        bool any_unkown = false;
        for (auto it = expr->op_begin(); it != expr->op_end(); it++) {
            auto sub_expr = std::static_pointer_cast<Expression>(*it);
            EvaluatedValue ret = run(sub_expr);
            if (ret == EvaluatedValue::True) {
                return expr->is_inverted() ? EvaluatedValue::False : EvaluatedValue::True;
            } else if (ret == EvaluatedValue::Unknown) {
                any_unkown = true;
            }
        }

        if (any_unkown) {
            return EvaluatedValue::Unknown;
        }
        // must have been all false
        return expr->is_inverted() ? EvaluatedValue::True : EvaluatedValue::False;
    } else if (std::dynamic_pointer_cast<AndExpr>(expr)) {
        bool any_unkown = false;
        for (auto it = expr->op_begin(); it != expr->op_end(); it++) {
            auto sub_expr = std::static_pointer_cast<Expression>(*it);
            EvaluatedValue ret = run(sub_expr);
            if (ret == EvaluatedValue::False) {
                return expr->is_inverted() ? EvaluatedValue::True : EvaluatedValue::False;
            } else if (ret == EvaluatedValue::Unknown) {
                any_unkown = true;
            }
        }

        if (any_unkown) {
            return EvaluatedValue::Unknown;
        }
        // must have been all true
        return expr->is_inverted() ? EvaluatedValue::False : EvaluatedValue::True;
    } else if (auto filter = std::dynamic_pointer_cast<FilterExpr>(expr)) {
        auto column = filter->get_column();
        if (false == column->matches_any(cTimestampTypes)) {
            return EvaluatedValue::Unknown;
        }

        for (auto range_it = m_timestamp_dict->tokenized_column_to_range_begin();
             range_it != m_timestamp_dict->tokenized_column_to_range_end();
             range_it++)
        {
            // Don't attempt to evaluate the timestamp index against columns with wildcard tokens.
            if (column->is_unresolved_descriptor()) {
                continue;
            }

            std::vector<std::string> const& tokens = range_it->first;
            auto const& descriptors = column->get_descriptor_list();
            if (tokens.size() != descriptors.size()) {
                continue;
            }

            bool matched = true;
            for (size_t i{0ULL}; i < descriptors.size(); ++i) {
                if (tokens[i] != descriptors[i].get_token()) {
                    matched = false;
                    break;
                }
            }
            if (false == matched) {
                continue;
            }

            auto const literal{filter->get_operand()};
            if (nullptr == literal) {
                return EvaluatedValue::Unknown;
            }

            EvaluatedValue ret{EvaluatedValue::Unknown};
            auto const filter_op{filter->get_operation()};
            int64_t int_literal{};
            double float_literal{};

            auto const encoding{range_it->second->get_timestamp_encoding()};
            switch (encoding) {
                case TimestampEntry::TimestampEncoding::DoubleEpoch:
                    if (false == literal->as_float(float_literal, filter_op)) {
                        return EvaluatedValue::Unknown;
                    }
                    ret = range_it->second->evaluate_filter(filter_op, float_literal);
                    break;
                case TimestampEntry::TimestampEncoding::Epoch:
                    if (auto const timestamp_literal{
                                std::dynamic_pointer_cast<TimestampLiteral>(literal)
                        };
                        nullptr != timestamp_literal)
                    {
                        int_literal = timestamp_literal->as_precision(
                                TimestampLiteral::Precision::Milliseconds
                        );
                    } else if (false == literal->as_int(int_literal, filter_op)) {
                        return EvaluatedValue::Unknown;
                    }
                    ret = range_it->second->evaluate_filter(filter_op, int_literal);
                    break;
                default:
                    return EvaluatedValue::Unknown;
            }

            if (ret == EvaluatedValue::True) {
                return filter->is_inverted() ? EvaluatedValue::False : EvaluatedValue::True;
            } else if (ret == EvaluatedValue::False) {
                return filter->is_inverted() ? EvaluatedValue::True : EvaluatedValue::False;
            }
            return EvaluatedValue::Unknown;
        }
        return EvaluatedValue::Unknown;
    } else {
        return EvaluatedValue::Unknown;
    }
}
}  // namespace clp_s::search
