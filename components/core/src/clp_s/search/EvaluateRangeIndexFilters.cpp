#include "EvaluateRangeIndexFilters.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "../../clp/ffi/EncodedTextAst.hpp"
#include "../../clp/ffi/ir_stream/search/utils.hpp"
#include "../../clp/ffi/Value.hpp"
#include "../../clp/ir/types.hpp"
#include "../archive_constants.hpp"
#include "../ArchiveReaderAdaptor.hpp"
#include "ast/AndExpr.hpp"
#include "ast/ColumnDescriptor.hpp"
#include "ast/ConstantProp.hpp"
#include "ast/EmptyExpr.hpp"
#include "ast/Expression.hpp"
#include "ast/FilterExpr.hpp"
#include "ast/Integral.hpp"
#include "ast/OrExpr.hpp"
#include "ast/OrOfAndForm.hpp"

using clp::ffi::ir_stream::search::evaluate_filter_against_literal_type_value_pair;

namespace clp_s::search {
auto EvaluateRangeIndexFilters::run(std::shared_ptr<ast::Expression>& expr)
        -> std::shared_ptr<ast::Expression> {
    bool must_renormalize{false};
    std::vector<std::pair<ast::Expression*, std::optional<ast::OpList::iterator>>> work_list;
    work_list.emplace_back(expr.get(), std::nullopt);
    while (false == work_list.empty()) {
        auto const [cur_expr, parent_it] = work_list.back();
        work_list.pop_back();
        if (cur_expr->has_only_expression_operands()) {
            for (auto it = cur_expr->op_begin(); it != cur_expr->op_end(); ++it) {
                work_list.emplace_back(static_cast<ast::Expression*>(it->get()), it);
            }
        } else if (auto filter_expr = dynamic_cast<ast::FilterExpr*>(cur_expr);
                   nullptr != filter_expr)
        {
            if (constants::cRangeIndexNamespace == filter_expr->get_column()->get_namespace()) {
                evaluate_and_rewrite_filter(filter_expr, parent_it, expr);
                must_renormalize = true;
            }
        }
    }

    if (must_renormalize) {
        ast::OrOfAndForm standardize_pass;
        expr = standardize_pass.run(expr);
        ast::ConstantProp constant_prop;
        expr = constant_prop.run(expr);
    }
    return expr;
}

void EvaluateRangeIndexFilters::evaluate_and_rewrite_filter(
        ast::FilterExpr* filter_expr,
        std::optional<ast::OpList::iterator> parent_it,
        std::shared_ptr<ast::Expression>& ast_root
) const {
    std::vector<std::pair<size_t, size_t>> matching_ranges;
    for (auto const& range : m_range_index) {
        if (evaluate_filter(filter_expr, range.fields)) {
            matching_ranges.emplace_back(range.start_index, range.end_index);
        }
    }

    auto replacement_expr{ast::EmptyExpr::create()};
    if (false == matching_ranges.empty()) {
        auto log_event_idx_col{ast::ColumnDescriptor::create_from_escaped_tokens(
                {std::string{constants::cLogEventIdxName}},
                constants::cDefaultNamespace
        )};
        log_event_idx_col->set_subtree_type(std::string{constants::cMetadataSubtreeType});
        log_event_idx_col->set_matching_type(ast::LiteralType::IntegerT);
        replacement_expr = ast::OrExpr::create();

        auto add_range_to_filter = [&](std::pair<size_t, size_t> const& range) {
            auto begin_literal{ast::Integral::create_from_int(range.first)};
            auto end_literal{ast::Integral::create_from_int(range.second)};
            auto begin_filter{ast::FilterExpr::create(
                    log_event_idx_col,
                    ast::FilterOperation::GTE,
                    begin_literal
            )};
            auto end_filter{ast::FilterExpr::create(
                    log_event_idx_col,
                    ast::FilterOperation::LT,
                    end_literal
            )};
            auto range_filter{ast::AndExpr::create(begin_filter, end_filter)};
            range_filter->copy_append(replacement_expr.get());
        };

        std::optional<std::pair<size_t, size_t>> cur_range;
        for (auto const& matching_range : matching_ranges) {
            if (false == cur_range.has_value()) {
                cur_range.emplace(matching_range);
                continue;
            }

            if (cur_range.value().second == matching_range.first) {
                cur_range.value().second = matching_range.second;
                continue;
            }

            add_range_to_filter(cur_range.value());
            cur_range.emplace(matching_range);
        }

        if (cur_range.has_value()) {
            add_range_to_filter(cur_range.value());
        }
    }

    if (false == parent_it.has_value()) {
        ast_root = replacement_expr;
    } else {
        replacement_expr->copy_replace(filter_expr->get_parent(), parent_it.value());
    }
}

auto EvaluateRangeIndexFilters::evaluate_filter(
        ast::FilterExpr* filter_expr,
        nlohmann::json const& fields
) const -> bool {
    auto const col{filter_expr->get_column()};
    auto const operand{filter_expr->get_operand()};
    std::vector<std::pair<ast::DescriptorList::iterator, nlohmann::json const&>> work_list;
    work_list.emplace_back(col->descriptor_begin(), fields);
    // Allow prefix wildcard to match zero tokens.
    if (col->descriptor_begin() != col->descriptor_end() && col->descriptor_begin()->wildcard()) {
        work_list.emplace_back(++col->descriptor_begin(), fields);
    }

    auto evaluate_expr
            = [&](ast::LiteralType type, std::optional<clp::ffi::Value> const& value) -> bool {
        auto ret{evaluate_filter_against_literal_type_value_pair(
                filter_expr,
                type,
                value,
                m_case_sensitive_match
        )};
        return false == ret.has_error() && filter_expr->is_inverted() != ret.value();
    };

    while (false == work_list.empty()) {
        auto [cur_it, cur_field] = work_list.back();
        work_list.pop_back();
        if (col->descriptor_end() == cur_it) {
            switch (cur_field.type()) {
                case nlohmann::json::value_t::boolean:
                    if (col->matches_type(ast::LiteralType::BooleanT)) {
                        std::optional<clp::ffi::Value> bool_value{
                                clp::ffi::Value{cur_field.template get<bool>()}
                        };
                        if (evaluate_expr(ast::LiteralType::BooleanT, bool_value)) {
                            return true;
                        }
                    }
                    break;
                case nlohmann::json::value_t::number_integer:
                    if (col->matches_type(ast::LiteralType::IntegerT)) {
                        std::optional<clp::ffi::Value> int_value{
                                clp::ffi::Value{cur_field.template get<int64_t>()}
                        };
                        if (evaluate_expr(ast::LiteralType::IntegerT, int_value)) {
                            return true;
                        }
                    }
                    break;
                case nlohmann::json::value_t::number_unsigned:
                    if (col->matches_type(ast::LiteralType::IntegerT)) {
                        // TODO: Remove static cast once we add full support for large unsigned
                        // values.
                        std::optional<clp::ffi::Value> int_value{clp::ffi::Value{
                                static_cast<int64_t>(cur_field.template get<uint64_t>())
                        }};
                        if (evaluate_expr(ast::LiteralType::IntegerT, int_value)) {
                            return true;
                        }
                    }
                    break;
                case nlohmann::json::value_t::number_float:
                    if (col->matches_type(ast::LiteralType::FloatT)) {
                        std::optional<clp::ffi::Value> float_value{
                                clp::ffi::Value{cur_field.template get<double>()}
                        };
                        if (evaluate_expr(ast::LiteralType::FloatT, float_value)) {
                            return true;
                        }
                    }
                    break;
                case nlohmann::json::value_t::string: {
                    if (false
                        == col->matches_any(
                                ast::LiteralType::VarStringT | ast::LiteralType::ClpStringT
                        ))
                    {
                        break;
                    }
                    auto tmp_string{cur_field.template get<std::string>()};
                    bool contains_space{std::string::npos != tmp_string.find(' ')};
                    if (false == contains_space) {
                        std::optional<clp::ffi::Value> str_value{
                                clp::ffi::Value{std::move(tmp_string)}
                        };
                        if (evaluate_expr(ast::LiteralType::VarStringT, str_value)) {
                            return true;
                        }
                    } else {
                        std::optional<clp::ffi::Value> const str_value{clp::ffi::Value{
                                clp::ffi::EncodedTextAst<clp::ir::four_byte_encoded_variable_t>::
                                        parse_and_encode_from(tmp_string)
                        }};
                        if (evaluate_expr(ast::LiteralType::ClpStringT, str_value)) {
                            return true;
                        }
                    }
                } break;
                case nlohmann::json::value_t::null:
                    if (col->matches_type(ast::LiteralType::NullT)) {
                        std::optional<clp::ffi::Value> null_value{clp::ffi::Value{}};
                        if (evaluate_expr(ast::LiteralType::NullT, null_value)) {
                            return true;
                        }
                    }
                    break;
                case nlohmann::json::value_t::array:
                    // TODO: Add array search call once
                    // `evaluate_filter_against_literal_type_value_pair` adds support for array
                    // search.
                case nlohmann::json::value_t::object:
                    // TODO: Add object filter once we add `LiteralType::ObjectT`.
                case nlohmann::json::value_t::discarded:
                default:
                    break;
            }
        } else if (cur_field.is_object() && cur_it->wildcard()) {
            auto cur_it_tmp = cur_it++;
            // Allow wildcard to continue matching tokens or continue on to next descriptor token.
            for (auto const& field : cur_field) {
                work_list.emplace_back(cur_it_tmp, field);
                work_list.emplace_back(cur_it, field);
            }
        } else if (cur_field.is_object() && cur_field.contains(cur_it->get_token())) {
            auto const& next_field(cur_field.at(cur_it->get_token()));
            work_list.emplace_back(++cur_it, next_field);

            // Allow wildcard to match zero tokens.
            if (col->descriptor_end() != cur_it && cur_it->wildcard()) {
                work_list.emplace_back(++cur_it, next_field);
            }
        }
    }
    return false;
}
}  // namespace clp_s::search
