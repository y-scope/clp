#include "QueryHandlerImpl.hpp"

#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <outcome/outcome.hpp>

#include "../../../../clp_s/archive_constants.hpp"
#include "../../../../clp_s/search/ast/ColumnDescriptor.hpp"
#include "../../../../clp_s/search/ast/ConvertToExists.hpp"
#include "../../../../clp_s/search/ast/EmptyExpr.hpp"
#include "../../../../clp_s/search/ast/Expression.hpp"
#include "../../../../clp_s/search/ast/FilterExpr.hpp"
#include "../../../../clp_s/search/ast/Literal.hpp"
#include "../../../../clp_s/search/ast/NarrowTypes.hpp"
#include "../../../../clp_s/search/ast/OrOfAndForm.hpp"
#include "../../../../clp_s/search/ast/SearchUtils.hpp"
#include "../../KeyValuePairLogEvent.hpp"
#include "../../SchemaTree.hpp"
#include "AstEvaluationResult.hpp"
#include "ErrorCode.hpp"

namespace clp::ffi::ir_stream::search {
namespace {
using clp_s::search::ast::ColumnDescriptor;
using clp_s::search::ast::EmptyExpr;
using clp_s::search::ast::Expression;
using clp_s::search::ast::FilterExpr;
using clp_s::search::ast::LiteralTypeBitmask;

/**
 * Pre-processes a search query by applying several transformation passes.
 * @param query
 * @return A result containing the transformed
 * - ErrorCodeEnum::QueryExpressionIsNull if `query` is nullptr.
 * - ErrorCodeEnum::QueryTransformationPassFailed if any of the transformation pass failed.
 */
[[nodiscard]] auto preprocess_query(std::shared_ptr<Expression> query)
        -> outcome_v2::std_result<std::shared_ptr<Expression>>;

/**
 * Creates projected columns and column-to-original-key map from the given projections.
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
        std::vector<std::pair<std::string, LiteralTypeBitmask>> const& projections
)
        -> outcome_v2::std_result<std::pair<
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
        -> outcome_v2::std_result<std::pair<
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
) -> outcome_v2::std_result<void>;

/**
 * @param key_namespace
 * @return Whether `key_namespace` is auto-generated or user-generated, or std::nullopt if the
 * namespace is unrecognized.
 */
[[nodiscard]] auto is_auto_generated(std::string_view key_namespace) -> std::optional<bool>;

auto preprocess_query(std::shared_ptr<Expression> query)
        -> outcome_v2::std_result<std::shared_ptr<Expression>> {
    if (nullptr == query) {
        return ErrorCode{ErrorCodeEnum::QueryExpressionIsNull};
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
        std::vector<std::pair<std::string, LiteralTypeBitmask>> const& projections
)
        -> outcome_v2::std_result<std::pair<
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
        -> outcome_v2::std_result<std::pair<
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
        it->second.emplace_back(
                OUTCOME_TRYX(QueryHandlerImpl::ColumnDescriptorTokenIterator::create(col))
        );
    }

    OUTCOME_TRYV(initialize_partial_resolution_from_search_ast(
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
) -> outcome_v2::std_result<void> {
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

        auto const begin_token_it{
                OUTCOME_TRYX(QueryHandlerImpl::ColumnDescriptorTokenIterator::create(col))
        };
        it->second.emplace_back(begin_token_it);
        if (false == begin_token_it.is_last() && begin_token_it.is_wildcard()) {
            // To handle the case where the prefix wildcard matches nothing
            it->second.emplace_back(OUTCOME_TRYX(begin_token_it.next()));
        }
    }

    return outcome_v2::success();
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
}  // namespace

auto QueryHandlerImpl::create(
        std::shared_ptr<Expression> query,
        std::vector<std::pair<std::string, LiteralTypeBitmask>> const& projections,
        bool case_sensitive_match
) -> outcome_v2::std_result<QueryHandlerImpl> {
    query = OUTCOME_TRYX(preprocess_query(query));
    auto [projected_columns, projected_column_to_original_key]
            = OUTCOME_TRYX(create_projected_columns_and_projection_map(projections));
    auto [auto_gen_namespace_partial_resolutions, user_gen_namespace_partial_resolutions]
            = OUTCOME_TRYX(
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

// TODO: Fix clang-tidy
// NOLINTNEXTLINE(*)
auto QueryHandlerImpl::evaluate_node_id_value_pairs(
        [[maybe_unused]] KeyValuePairLogEvent::NodeIdValuePairs const& auto_gen_node_id_value_pairs,
        [[maybe_unused]] KeyValuePairLogEvent::NodeIdValuePairs const& user_gen_node_id_value_pairs
) -> outcome_v2::std_result<AstEvaluationResult> {
    return ErrorCode{ErrorCodeEnum::MethodNotImplemented};
}
}  // namespace clp::ffi::ir_stream::search
