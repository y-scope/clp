#include "EvaluateTimestampIndex.hpp"

#include "AndExpr.hpp"
#include "FilterExpr.hpp"
#include "Integral.hpp"
#include "OrExpr.hpp"

namespace clp_s::search {
constexpr LiteralTypeBitmask cDateTypes = cIntegralTypes | EpochDateT;

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

            EvaluatedValue ret;
            // this is safe after type narrowing because all DateType literals are either
            // Integral or a derived class of Integral
            Integral64 literal = std::static_pointer_cast<Integral>(filter->get_operand())->get();
            if (std::holds_alternative<int64_t>(literal)) {
                ret = range_it->second->evaluate_filter(
                        filter->get_operation(),
                        std::get<int64_t>(literal)
                );
            } else {
                ret = range_it->second->evaluate_filter(
                        filter->get_operation(),
                        std::get<double>(literal)
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
