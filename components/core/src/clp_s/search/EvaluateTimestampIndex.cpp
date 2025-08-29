#include "EvaluateTimestampIndex.hpp"

#include "ast/AndExpr.hpp"
#include "ast/Expression.hpp"
#include "ast/FilterExpr.hpp"
#include "ast/FilterOperation.hpp"
#include "ast/Integral.hpp"
#include "ast/Literal.hpp"
#include "ast/OrExpr.hpp"

using clp_s::search::ast::AndExpr;
using clp_s::search::ast::Expression;
using clp_s::search::ast::FilterExpr;
using clp_s::search::ast::FilterOperation;
using clp_s::search::ast::Integral;
using clp_s::search::ast::Integral64;
using clp_s::search::ast::literal_type_bitmask_t;
using clp_s::search::ast::OrExpr;

namespace clp_s::search {
constexpr literal_type_bitmask_t cDateTypes = search::ast::cIntegralTypes | search::ast::EpochDateT;

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
        if (false == column->matches_any(cDateTypes)) {
            return EvaluatedValue::Unknown;
        }

        for (auto range_it = m_timestamp_dict->tokenized_column_to_range_begin();
             range_it != m_timestamp_dict->tokenized_column_to_range_end();
             range_it++)
        {
            std::vector<std::string> const& tokens = range_it->first;
            auto const& descriptors = column->get_descriptor_list();
            // TODO: handle wildcard matching; the initial check on timestamp index happens
            // before schema matching, so
            if (tokens.size() != descriptors.size()) {
                continue;
            }

            bool matched = true;
            for (size_t i = 0; i < descriptors.size(); ++i) {
                if (tokens[i] != descriptors[i].get_token()) {
                    matched = false;
                    break;
                }
            }
            if (false == matched) {
                continue;
            }

            // Leave handling EXISTS/NEXISTS to schema matching.
            auto const filter_op{filter->get_operation()};
            if (FilterOperation::EXISTS == filter_op || FilterOperation::NEXISTS == filter_op) {
                return EvaluatedValue::Unknown;
            }

            auto const literal{filter->get_operand()};
            // Defend against future FilterOperation types that don't take a literal.
            if (nullptr == literal) {
                return EvaluatedValue::Unknown;
            }

            // Handle the case where the literal is a pure wildcard.
            if (literal->as_any(filter_op)) {
                switch (filter_op) {
                    case FilterOperation::EQ:
                        return filter->is_inverted() ? EvaluatedValue::False : EvaluatedValue::True;
                    case FilterOperation::NEQ:
                        return filter->is_inverted() ? EvaluatedValue::True : EvaluatedValue::False;
                    default:
                        // Not reachable.
                        return EvaluatedValue::Unknown;
                }
            }

            EvaluatedValue ret{EvaluatedValue::Unknown};
            // This should be safe after type narrowing and checking that the literal exists and is
            // not a pure wildcard because all DateType literals are either Integral or a derived
            // class of Integral. Still, we check whether the literal is an Integral in case this
            // assumption breaks for future versions of the AST.
            auto integral_literal{std::dynamic_pointer_cast<Integral>(literal)};
            if (nullptr == integral_literal) {
                return EvaluatedValue::Unknown;
            }
            Integral64 integral = integral_literal->get();
            if (std::holds_alternative<int64_t>(integral)) {
                ret = range_it->second->evaluate_filter(
                        filter->get_operation(),
                        std::get<int64_t>(integral)
                );
            } else {
                ret = range_it->second->evaluate_filter(
                        filter->get_operation(),
                        std::get<double>(integral)
                );
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
