#include "QueryHandlerImpl.hpp"

#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include "../../../../clp_s/archive_constants.hpp"
#include "../../../../clp_s/search/ast/AndExpr.hpp"
#include "../../../../clp_s/search/ast/ColumnDescriptor.hpp"
#include "../../../../clp_s/search/ast/ConvertToExists.hpp"
#include "../../../../clp_s/search/ast/EmptyExpr.hpp"
#include "../../../../clp_s/search/ast/Expression.hpp"
#include "../../../../clp_s/search/ast/FilterExpr.hpp"
#include "../../../../clp_s/search/ast/Literal.hpp"
#include "../../../../clp_s/search/ast/NarrowTypes.hpp"
#include "../../../../clp_s/search/ast/OrExpr.hpp"
#include "../../../../clp_s/search/ast/OrOfAndForm.hpp"
#include "../../../../clp_s/search/ast/SearchUtils.hpp"
#include "../../../../clp_s/search/ast/Value.hpp"
#include "../../../TraceableException.hpp"
#include "../../KeyValuePairLogEvent.hpp"
#include "../../SchemaTree.hpp"
#include "../../Value.hpp"
#include "AstEvaluationResult.hpp"
#include "ErrorCode.hpp"
#include "utils.hpp"

namespace clp::ffi::ir_stream::search {
namespace {
using clp_s::search::ast::ColumnDescriptor;
using clp_s::search::ast::EmptyExpr;
using clp_s::search::ast::Expression;
using clp_s::search::ast::FilterExpr;
using clp_s::search::ast::literal_type_bitmask_t;

/**
 * Pre-processes a search query by applying several transformation passes.
 * @param query
 * @return A result containing the transformed query on success, or an error code indicating the
 * failure:
 * - ErrorCodeEnum::QueryTransformationPassFailed if any of the transformation pass failed.
 */
[[nodiscard]] auto preprocess_query(std::shared_ptr<Expression> query)
        -> ystdlib::error_handling::Result<std::shared_ptr<Expression>>;

/**
 * Creates column descriptors and column-to-original-key map from the given projections.
 * @param projections
 * @return A result containing a pair or an error code indicating the failure:
 * - The pair:
 *   - A vector of projected columns.
 *   - A projected-column-to-original-key map.
 * - The possible error codes:
 *   - ErrorCodeEnum::DuplicateProjectedColumn if `projections` contains duplicated keys.
 *   - ErrorCodeEnum::ColumnTokenizationFailure if failed to tokenize a projection key.
 *   - ErrorCodeEnum::ProjectionColumnDescriptorCreationFailure if failed to create a column
 *     descriptor for the projected key.
 */
[[nodiscard]] auto create_projected_columns_and_projection_map(
        std::vector<std::pair<std::string, literal_type_bitmask_t>> const& projections
)
        -> ystdlib::error_handling::Result<std::pair<
                std::vector<std::shared_ptr<ColumnDescriptor>>,
                QueryHandlerImpl::ProjectionMap>>;

/**
 * Creates initial partial resolutions for the given query and projections.
 * @param query
 * @param projected_column_to_original_key
 * @return A result containing a pair or an error code indicating the failure:
 * - The pair:
 *   - The partial resolution for auto-generated keys.
 *   - The partial resolution for user-generated keys.
 * - The possible error codes:
 *   - Forwards `initialize_partial_resolution_from_search_ast`'s return values.
 *   - Forwards `QueryHandlerImpl::ColumnDescriptorTokenIterator::create`'s return values.
 *   - Forwards `QueryHandlerImpl::ColumnDescriptorTokenIterator::next`'s return values.
 */
[[nodiscard]] auto create_initial_partial_resolutions(
        std::shared_ptr<Expression> const& query,
        QueryHandlerImpl::ProjectionMap const& projected_column_to_original_key
)
        -> ystdlib::error_handling::Result<std::pair<
                QueryHandlerImpl::PartialResolutionMap,
                QueryHandlerImpl::PartialResolutionMap>>;

/**
 * Initializes partial resolutions from a search AST.
 * @param root The root of the search AST.
 * @param auto_gen_namespace_partial_resolutions Returns initialized auto-gen partial resolutions.
 * @param user_gen_namespace_partial_resolutions Returns initialized user-gen partial resolutions.
 * @return A void result on success, or an error code indicating the failure:
 * - ErrorCodeEnum::AstDynamicCastFailure if failed to dynamically cast an AST node to a target
 *   type.
 */
[[nodiscard]] auto initialize_partial_resolution_from_search_ast(
        std::shared_ptr<Expression> const& root,
        QueryHandlerImpl::PartialResolutionMap& auto_gen_namespace_partial_resolutions,
        QueryHandlerImpl::PartialResolutionMap& user_gen_namespace_partial_resolutions
) -> ystdlib::error_handling::Result<void>;

/**
 * @param key_namespace
 * @return Whether `key_namespace` is auto-generated or user-generated, or std::nullopt if the
 * namespace is unrecognized.
 */
[[nodiscard]] auto is_auto_generated(std::string_view key_namespace) -> std::optional<bool>;

/**
 * Evaluates a filter expression against the given node-ID-value pair.
 * @param filter_expr
 * @param node_id
 * @param value
 * @param schema_tree
 * @param case_sensitive_match
 * @return A result containing the evaluation result on success, or an error code indicating the
 * failure:
 * - ErrorCodeEnum::AstEvaluationInvariantViolation if a `TraceableException` is caught during
 *   evaluation.
 * - Forwards `evaluate_filter_against_literal_type_value_pair`'s return values.
 */
[[nodiscard]] auto evaluate_filter_against_node_id_value_pair(
        clp_s::search::ast::FilterExpr* filter_expr,
        SchemaTree::Node::id_t node_id,
        std::optional<Value> const& value,
        SchemaTree const& schema_tree,
        bool case_sensitive_match
) -> ystdlib::error_handling::Result<AstEvaluationResult>;

/**
 * Evaluates a wildcard filter expression.
 * @param filter_expr
 * @param node_id_value_pairs
 * @param schema_tree
 * @param case_sensitive_match
 * @return A result containing the evaluation result on success, or an error code indicating the
 * failure:
 * - Forwards `evaluate_filter_against_node_id_value_pair`'s return values.
 */
[[nodiscard]] auto evaluate_wildcard_filter(
        clp_s::search::ast::FilterExpr* filter_expr,
        KeyValuePairLogEvent::NodeIdValuePairs const& node_id_value_pairs,
        SchemaTree const& schema_tree,
        bool case_sensitive_match
) -> ystdlib::error_handling::Result<AstEvaluationResult>;

auto preprocess_query(std::shared_ptr<Expression> query)
        -> ystdlib::error_handling::Result<std::shared_ptr<Expression>> {
    if (nullptr == query) {
        return query;
    }

    if (nullptr != std::dynamic_pointer_cast<EmptyExpr>(query)) {
        return query;
    }

    try {
        if (query = clp_s::search::ast::OrOfAndForm{}.run(query);
            nullptr != std::dynamic_pointer_cast<EmptyExpr>(query))
        {
            return query;
        }

        if (query = clp_s::search::ast::NarrowTypes{}.run(query);
            nullptr != std::dynamic_pointer_cast<EmptyExpr>(query))
        {
            return query;
        }

        return clp_s::search::ast::ConvertToExists{}.run(query);
    } catch (std::exception const& ex) {
        return ErrorCode{ErrorCodeEnum::QueryTransformationPassFailed};
    }
}

auto create_projected_columns_and_projection_map(
        std::vector<std::pair<std::string, literal_type_bitmask_t>> const& projections
)
        -> ystdlib::error_handling::Result<std::pair<
                std::vector<std::shared_ptr<ColumnDescriptor>>,
                QueryHandlerImpl::ProjectionMap>> {
    std::unordered_set<std::string_view> unique_projected_columns;
    std::vector<std::shared_ptr<ColumnDescriptor>> projected_columns;
    QueryHandlerImpl::ProjectionMap projected_column_to_original_key;

    for (auto const& [key, types] : projections) {
        if (unique_projected_columns.contains(key)) {
            return ErrorCode{ErrorCodeEnum::DuplicateProjectedColumn};
        }
        unique_projected_columns.emplace(key);

        std::vector<std::string> descriptor_tokens;
        std::string descriptor_namespace;
        if (false
            == clp_s::search::ast::tokenize_column_descriptor(
                    key,
                    descriptor_tokens,
                    descriptor_namespace
            ))
        {
            return ErrorCode{ErrorCodeEnum::ColumnTokenizationFailure};
        }

        try {
            auto column_descriptor{clp_s::search::ast::ColumnDescriptor::create_from_escaped_tokens(
                    descriptor_tokens,
                    descriptor_namespace
            )};
            column_descriptor->set_matching_types(types);
            if (column_descriptor->is_unresolved_descriptor()
                || column_descriptor->get_descriptor_list().empty())
            {
                return ErrorCode{ErrorCodeEnum::ProjectionColumnDescriptorCreationFailure};
            }
            projected_column_to_original_key.emplace(column_descriptor.get(), key);
            projected_columns.emplace_back(std::move(column_descriptor));
        } catch (std::exception const& e) {
            return ErrorCode{ErrorCodeEnum::ProjectionColumnDescriptorCreationFailure};
        }
    }

    return {std::move(projected_columns), std::move(projected_column_to_original_key)};
}

auto create_initial_partial_resolutions(
        std::shared_ptr<Expression> const& query,
        QueryHandlerImpl::ProjectionMap const& projected_column_to_original_key
)
        -> ystdlib::error_handling::Result<std::pair<
                QueryHandlerImpl::PartialResolutionMap,
                QueryHandlerImpl::PartialResolutionMap>> {
    QueryHandlerImpl::PartialResolutionMap auto_gen_namespace_partial_resolutions;
    QueryHandlerImpl::PartialResolutionMap user_gen_namespace_partial_resolutions;
    for (auto const& [col, key] : projected_column_to_original_key) {
        auto const optional_is_auto_gen{is_auto_generated(col->get_namespace())};
        if (false == optional_is_auto_gen.has_value()) {
            // Ignore unrecognized namespace
            continue;
        }
        auto& partial_resolutions{
                optional_is_auto_gen.value() ? auto_gen_namespace_partial_resolutions
                                             : user_gen_namespace_partial_resolutions
        };
        auto [it, inserted] = partial_resolutions.try_emplace(
                SchemaTree::cRootId,
                std::vector<QueryHandlerImpl::ColumnDescriptorTokenIterator>{}
        );
        it->second.emplace_back(YSTDLIB_ERROR_HANDLING_TRYX(
                QueryHandlerImpl::ColumnDescriptorTokenIterator::create(col)
        ));
    }

    YSTDLIB_ERROR_HANDLING_TRYV(initialize_partial_resolution_from_search_ast(
            query,
            auto_gen_namespace_partial_resolutions,
            user_gen_namespace_partial_resolutions
    ));

    return {std::move(auto_gen_namespace_partial_resolutions),
            std::move(user_gen_namespace_partial_resolutions)};
}

auto initialize_partial_resolution_from_search_ast(
        std::shared_ptr<Expression> const& root,
        QueryHandlerImpl::PartialResolutionMap& auto_gen_namespace_partial_resolutions,
        QueryHandlerImpl::PartialResolutionMap& user_gen_namespace_partial_resolutions
) -> ystdlib::error_handling::Result<void> {
    if (nullptr == root) {
        return ystdlib::error_handling::success();
    }

    std::vector<Expression*> ast_dfs_stack;
    ast_dfs_stack.emplace_back(root.get());
    while (false == ast_dfs_stack.empty()) {
        auto* expr{ast_dfs_stack.back()};
        ast_dfs_stack.pop_back();
        if (expr->has_only_expression_operands()) {
            for (auto it{expr->op_begin()}; it != expr->op_end(); ++it) {
                auto* child_expr{dynamic_cast<Expression*>(it->get())};
                if (nullptr == child_expr) {
                    return ErrorCode{ErrorCodeEnum::AstDynamicCastFailure};
                }
                ast_dfs_stack.emplace_back(child_expr);
            }
            continue;
        }

        auto* filter{dynamic_cast<FilterExpr*>(expr)};
        if (nullptr == filter) {
            continue;
        }

        auto* col{filter->get_column().get()};
        if (col->is_pure_wildcard() || col->get_descriptor_list().empty()) {
            // If the column is a single wildcard, we will resolve it dynamically during the AST
            // evaluation.
            continue;
        }

        auto const optional_is_auto_gen{is_auto_generated(col->get_namespace())};
        if (false == optional_is_auto_gen.has_value()) {
            // Ignore unrecognized namespace
            continue;
        }
        auto& partial_resolutions{
                optional_is_auto_gen.value() ? auto_gen_namespace_partial_resolutions
                                             : user_gen_namespace_partial_resolutions
        };

        auto [it, inserted] = partial_resolutions.try_emplace(
                SchemaTree::cRootId,
                std::vector<QueryHandlerImpl::ColumnDescriptorTokenIterator>{}
        );

        auto const begin_token_it{YSTDLIB_ERROR_HANDLING_TRYX(
                QueryHandlerImpl::ColumnDescriptorTokenIterator::create(col)
        )};
        it->second.emplace_back(begin_token_it);
        if (false == begin_token_it.is_last() && begin_token_it.is_wildcard()) {
            // To handle the case where the prefix wildcard matches nothing
            it->second.emplace_back(YSTDLIB_ERROR_HANDLING_TRYX(begin_token_it.next()));
        }
    }

    return ystdlib::error_handling::success();
}

auto is_auto_generated(std::string_view key_namespace) -> std::optional<bool> {
    if (clp_s::constants::cAutogenNamespace == key_namespace) {
        return true;
    }
    if (clp_s::constants::cDefaultNamespace == key_namespace) {
        return false;
    }
    return std::nullopt;
}

auto evaluate_filter_against_node_id_value_pair(
        clp_s::search::ast::FilterExpr* filter_expr,
        SchemaTree::Node::id_t node_id,
        std::optional<Value> const& value,
        SchemaTree const& schema_tree,
        bool case_sensitive_match
) -> ystdlib::error_handling::Result<AstEvaluationResult> {
    try {
        auto const node_type{schema_tree.get_node(node_id).get_type()};
        auto const literal_type{schema_tree_node_type_value_pair_to_literal_type(node_type, value)};
        if (false == filter_expr->get_column()->matches_type(literal_type)) {
            return AstEvaluationResult::Pruned;
        }
        auto const evaluation_result{evaluate_filter_against_literal_type_value_pair(
                filter_expr,
                literal_type,
                value,
                case_sensitive_match
        )};
        if (false == evaluation_result.has_error()) {
            return evaluation_result.value() ? AstEvaluationResult::True
                                             : AstEvaluationResult::False;
        }
        if (ErrorCode{ErrorCodeEnum::LiteralTypeUnsupported} == evaluation_result.error()) {
            // Evaluations on unsupported literal types are considered `AstEvaluationResult::False`.
            return AstEvaluationResult::False;
        }
        return evaluation_result.error();
    } catch (TraceableException const& ex) {
        return ErrorCode{ErrorCodeEnum::AstEvaluationInvariantViolation};
    }
}

auto evaluate_wildcard_filter(
        clp_s::search::ast::FilterExpr* filter_expr,
        KeyValuePairLogEvent::NodeIdValuePairs const& node_id_value_pairs,
        SchemaTree const& schema_tree,
        bool case_sensitive_match
) -> ystdlib::error_handling::Result<AstEvaluationResult> {
    ast_evaluation_result_bitmask_t evaluation_results{};
    for (auto const& [node_id, value] : node_id_value_pairs) {
        auto const evaluation_result{
                YSTDLIB_ERROR_HANDLING_TRYX(evaluate_filter_against_node_id_value_pair(
                        filter_expr,
                        node_id,
                        value,
                        schema_tree,
                        case_sensitive_match
                ))
        };
        if (AstEvaluationResult::True == evaluation_result) {
            return AstEvaluationResult::True;
        }
        evaluation_results |= evaluation_result;
    }
    if ((evaluation_results & AstEvaluationResult::False) != 0) {
        return AstEvaluationResult::False;
    }
    return AstEvaluationResult::Pruned;
}
}  // namespace

auto QueryHandlerImpl::create(
        std::shared_ptr<Expression> query,
        std::vector<std::pair<std::string, literal_type_bitmask_t>> const& projections,
        bool case_sensitive_match
) -> ystdlib::error_handling::Result<QueryHandlerImpl> {
    query = YSTDLIB_ERROR_HANDLING_TRYX(preprocess_query(query));
    auto [projected_columns, projected_column_to_original_key]
            = YSTDLIB_ERROR_HANDLING_TRYX(create_projected_columns_and_projection_map(projections));
    auto [auto_gen_namespace_partial_resolutions, user_gen_namespace_partial_resolutions]
            = YSTDLIB_ERROR_HANDLING_TRYX(
                    create_initial_partial_resolutions(query, projected_column_to_original_key)
            );

    return QueryHandlerImpl{
            std::move(query),
            std::move(auto_gen_namespace_partial_resolutions),
            std::move(user_gen_namespace_partial_resolutions),
            std::move(projected_columns),
            std::move(projected_column_to_original_key),
            case_sensitive_match
    };
}

auto QueryHandlerImpl::evaluate_kv_pair_log_event(KeyValuePairLogEvent const& log_event)
        -> ystdlib::error_handling::Result<AstEvaluationResult> {
    if (nullptr == m_query) {
        return AstEvaluationResult::True;
    }

    if (m_is_empty_query) {
        return AstEvaluationResult::False;
    }

    std::optional<AstEvaluationResult> optional_evaluation_result;
    m_ast_dfs_stack.clear();
    push_to_ast_dfs_stack(YSTDLIB_ERROR_HANDLING_TRYX(AstExprIterator::create(m_query.get())));
    while (false == m_ast_dfs_stack.empty()) {
        YSTDLIB_ERROR_HANDLING_TRYV(
                advance_ast_dfs_evaluation(log_event, optional_evaluation_result)
        );
    }

    if (false == optional_evaluation_result.has_value()) {
        return ErrorCode{ErrorCodeEnum::AstEvaluationInvariantViolation};
    }

    return optional_evaluation_result.value();
}

auto QueryHandlerImpl::AstExprIterator::create(clp_s::search::ast::Value* expr)
        -> ystdlib::error_handling::Result<AstExprIterator> {
    if (auto* and_expr{dynamic_cast<clp_s::search::ast::AndExpr*>(expr)}; nullptr != and_expr) {
        return AstExprIterator{
                ExprVariant{and_expr},
                and_expr->op_begin(),
                and_expr->op_end(),
                and_expr->is_inverted()
        };
    }

    if (auto* or_expr{dynamic_cast<clp_s::search::ast::OrExpr*>(expr)}; nullptr != or_expr) {
        return AstExprIterator{
                ExprVariant{or_expr},
                or_expr->op_begin(),
                or_expr->op_end(),
                or_expr->is_inverted()
        };
    }

    if (auto* filter_expr{dynamic_cast<clp_s::search::ast::FilterExpr*>(expr)};
        nullptr != filter_expr)
    {
        return AstExprIterator{
                ExprVariant{filter_expr},
                filter_expr->op_begin(),
                filter_expr->op_end(),
                filter_expr->is_inverted()
        };
    }

    return ErrorCode{ErrorCodeEnum::ExpressionTypeUnexpected};
}

auto QueryHandlerImpl::AstExprIterator::next_op()
        -> std::optional<ystdlib::error_handling::Result<AstExprIterator>> {
    if (m_op_end_it == m_op_next_it) {
        return std::nullopt;
    }
    if (std::holds_alternative<clp_s::search::ast::FilterExpr*>(m_expr)) {
        return ErrorCode{ErrorCodeEnum::AttemptToIterateAstLeafExpr};
    }
    return create((m_op_next_it++)->get());
}

auto QueryHandlerImpl::evaluate_filter_expr(
        clp_s::search::ast::FilterExpr* filter_expr,
        KeyValuePairLogEvent const& log_event
) -> ystdlib::error_handling::Result<AstEvaluationResult> {
    auto* col{filter_expr->get_column().get()};

    if (col->is_pure_wildcard()) {
        auto const auto_gen_evaluation_result{YSTDLIB_ERROR_HANDLING_TRYX(evaluate_wildcard_filter(
                filter_expr,
                log_event.get_auto_gen_node_id_value_pairs(),
                log_event.get_auto_gen_keys_schema_tree(),
                m_case_sensitive_match
        ))};
        if (AstEvaluationResult::True == auto_gen_evaluation_result) {
            return AstEvaluationResult::True;
        }

        auto const user_gen_evaluation_result{YSTDLIB_ERROR_HANDLING_TRYX(evaluate_wildcard_filter(
                filter_expr,
                log_event.get_user_gen_node_id_value_pairs(),
                log_event.get_user_gen_keys_schema_tree(),
                m_case_sensitive_match
        ))};
        if (AstEvaluationResult::True == user_gen_evaluation_result) {
            return AstEvaluationResult::True;
        }

        if (AstEvaluationResult::Pruned == auto_gen_evaluation_result
            && AstEvaluationResult::Pruned == user_gen_evaluation_result)
        {
            return AstEvaluationResult::Pruned;
        }
        return AstEvaluationResult::False;
    }

    if (false == m_resolved_column_to_schema_tree_node_ids.contains(col)) {
        return AstEvaluationResult::Pruned;
    }

    auto const optional_is_auto_gen{is_auto_generated(col->get_namespace())};
    if (false == optional_is_auto_gen.has_value()) {
        return ErrorCode{ErrorCodeEnum::AstEvaluationInvariantViolation};
    }
    auto const& schema_tree{
            *optional_is_auto_gen ? log_event.get_auto_gen_keys_schema_tree()
                                  : log_event.get_user_gen_keys_schema_tree()
    };
    auto const& node_id_value_pairs{
            *optional_is_auto_gen ? log_event.get_auto_gen_node_id_value_pairs()
                                  : log_event.get_user_gen_node_id_value_pairs()
    };
    auto const& matchable_node_ids{m_resolved_column_to_schema_tree_node_ids.at(col)};

    ast_evaluation_result_bitmask_t evaluation_results{};
    for (auto const matchable_node_id : matchable_node_ids) {
        if (false == node_id_value_pairs.contains(matchable_node_id)) {
            continue;
        }
        auto const evaluation_result{
                YSTDLIB_ERROR_HANDLING_TRYX(evaluate_filter_against_node_id_value_pair(
                        filter_expr,
                        matchable_node_id,
                        node_id_value_pairs.at(matchable_node_id),
                        schema_tree,
                        m_case_sensitive_match
                ))
        };
        if (AstEvaluationResult::True == evaluation_result) {
            return AstEvaluationResult::True;
        }
        evaluation_results |= evaluation_result;
    }

    if (0 != (evaluation_results & AstEvaluationResult::False)) {
        return AstEvaluationResult::False;
    }
    return AstEvaluationResult::Pruned;
}

auto QueryHandlerImpl::pop_from_ast_dfs_stack_and_update_evaluation_results(
        clp::ffi::ir_stream::search::AstEvaluationResult evaluation_result,
        std::optional<AstEvaluationResult>& query_evaluation_result
) -> void {
    auto const is_inverted{m_ast_dfs_stack.back().first.is_inverted()};
    if (AstEvaluationResult::Pruned != evaluation_result && is_inverted) {
        evaluation_result = AstEvaluationResult::True == evaluation_result
                                    ? AstEvaluationResult::False
                                    : AstEvaluationResult::True;
    }
    m_ast_dfs_stack.pop_back();
    if (m_ast_dfs_stack.empty()) {
        query_evaluation_result.emplace(evaluation_result);
        return;
    }
    m_ast_dfs_stack.back().second |= evaluation_result;
}

auto QueryHandlerImpl::advance_ast_dfs_evaluation(
        KeyValuePairLogEvent const& log_event,
        std::optional<AstEvaluationResult>& query_evaluation_result
) -> ystdlib::error_handling::Result<void> {
    auto& [expr_it, evaluation_results] = m_ast_dfs_stack.back();
    if (auto* filter_expr{expr_it.as_filter_expr()}; nullptr != filter_expr) {
        pop_from_ast_dfs_stack_and_update_evaluation_results(
                YSTDLIB_ERROR_HANDLING_TRYX(evaluate_filter_expr(filter_expr, log_event)),
                query_evaluation_result
        );
        return ystdlib::error_handling::success();
    }

    if (auto const* and_expr{expr_it.as_and_expr()}; nullptr != and_expr) {
        // Handle `AndExpr` evaluation
        if (0 != (evaluation_results & AstEvaluationResult::Pruned)) {
            pop_from_ast_dfs_stack_and_update_evaluation_results(
                    AstEvaluationResult::Pruned,
                    query_evaluation_result
            );
            return ystdlib::error_handling::success();
        }
        if (0 != (evaluation_results & AstEvaluationResult::False)) {
            pop_from_ast_dfs_stack_and_update_evaluation_results(
                    AstEvaluationResult::False,
                    query_evaluation_result
            );
            return ystdlib::error_handling::success();
        }
        auto const optional_next_op_it{expr_it.next_op()};
        if (optional_next_op_it.has_value()) {
            push_to_ast_dfs_stack(YSTDLIB_ERROR_HANDLING_TRYX(optional_next_op_it.value()));
        } else {
            pop_from_ast_dfs_stack_and_update_evaluation_results(
                    AstEvaluationResult::True,
                    query_evaluation_result
            );
        }
        return ystdlib::error_handling::success();
    }

    // Handle `OrExpr` evaluation
    auto const* or_expr{expr_it.as_or_expr()};
    if (nullptr == or_expr) {
        return ErrorCode{ErrorCodeEnum::AstEvaluationInvariantViolation};
    }
    if (0 != (evaluation_results & AstEvaluationResult::True)) {
        pop_from_ast_dfs_stack_and_update_evaluation_results(
                AstEvaluationResult::True,
                query_evaluation_result
        );
        return ystdlib::error_handling::success();
    }
    auto const optional_next_op_it{expr_it.next_op()};
    if (optional_next_op_it.has_value()) {
        push_to_ast_dfs_stack(YSTDLIB_ERROR_HANDLING_TRYX(optional_next_op_it.value()));
        return ystdlib::error_handling::success();
    }
    if (0 != (evaluation_results & AstEvaluationResult::False)) {
        pop_from_ast_dfs_stack_and_update_evaluation_results(
                AstEvaluationResult::False,
                query_evaluation_result
        );
        return ystdlib::error_handling::success();
    }

    // All pruned
    pop_from_ast_dfs_stack_and_update_evaluation_results(
            AstEvaluationResult::Pruned,
            query_evaluation_result
    );
    return ystdlib::error_handling::success();
}
}  // namespace clp::ffi::ir_stream::search
