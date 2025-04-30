#include "QueryHandlerImpl.hpp"

#include <exception>
#include <memory>
#include <unordered_set>

#include <outcome/outcome.hpp>

#include "../../../../clp_s/archive_constants.hpp"
#include "../../../../clp_s/search/ast/ConvertToExists.hpp"
#include "../../../../clp_s/search/ast/EmptyExpr.hpp"
#include "../../../../clp_s/search/ast/Expression.hpp"
#include "../../../../clp_s/search/ast/FilterExpr.hpp"
#include "../../../../clp_s/search/ast/NarrowTypes.hpp"
#include "../../../../clp_s/search/ast/OrOfAndForm.hpp"
#include "../../../../clp_s/search/ast/SearchUtils.hpp"
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
 * Creates a column-descriptor-to-original-key map from the given projections.
 * @param projections
 * @return A result containing the constructed map on success, or an error code indicating the
 * failure:
 * - ErrorCodeEnum::DuplicateProjectedColumn if `projections` contains duplicated keys.
 * - ErrorCodeEnum::ColumnTokenizationFailure if failed to tokenize a projection key.
 * - ErrorCodeEnum::ProjectionColumnDescriptorCreationFailure if failed to create a column
 *   descriptor for the projected key.
 */
[[nodiscard]] auto create_projection_map(
        std::vector<std::pair<std::string, LiteralTypeBitmask>> const& projections
) -> outcome_v2::std_result<QueryHandlerImpl::ProjectionMap>;

/**
 * Creates initial partial resolutions for the given query and projections.
 * @param query
 * @param projected_column_to_original_key
 * @return A result containing a pair or an error code indicating the failure:
 * - The pair:
 *   - The partial resolution for auto-generated keys.
 *   - The partial resolution for user-generated keys.
 * - The possible error codes:
 *   - Forwards `is_auto_generated`'s return values.
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
 * @param key_namespace
 * @return A result containing a boolean indicating whether `key_namespace` is auto-generated or
 * user-generated on success, or an error code indicating the failure:
 * - ErrorCodeEnum::UnsupportedNamespace if the namespace is not supported.
 */
[[nodiscard]] auto is_auto_generated(std::string_view key_namespace)
        -> outcome_v2::std_result<bool>;

auto preprocess_query(std::shared_ptr<Expression> query)
        -> outcome_v2::std_result<std::shared_ptr<Expression>> {
    if (nullptr == query) {
        return ErrorCode{ErrorCodeEnum::QueryExpressionIsNull};
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

auto create_projection_map(
        std::vector<std::pair<std::string, LiteralTypeBitmask>> const& projections
) -> outcome_v2::std_result<QueryHandlerImpl::ProjectionMap> {
    std::unordered_set<std::string_view> unique_projected_columns;
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
            auto column_descriptor
                    = clp_s::search::ast::ColumnDescriptor::create_from_escaped_tokens(
                            descriptor_tokens,
                            descriptor_namespace
                    );
            column_descriptor->set_matching_types(types);
            if (column_descriptor->is_unresolved_descriptor()
                || column_descriptor->get_descriptor_list().empty())
            {
                return ErrorCode{ErrorCodeEnum::ProjectionColumnDescriptorCreationFailure};
            }
            projected_column_to_original_key.emplace(std::move(column_descriptor), key);
        } catch (std::exception const& e) {
            return ErrorCode{ErrorCodeEnum::ProjectionColumnDescriptorCreationFailure};
        }
    }

    return std::move(projected_column_to_original_key);
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
        bool const is_auto_gen{OUTCOME_TRYX(is_auto_generated(col->get_namespace()))};
        auto& partial_resolutions{
                is_auto_gen ? auto_gen_namespace_partial_resolutions
                            : user_gen_namespace_partial_resolutions
        };
        if (false == partial_resolutions.contains(SchemaTree::cRootId)) {
            partial_resolutions.emplace(
                    SchemaTree::cRootId,
                    std::vector<QueryHandlerImpl::ColumnDescriptorTokenIterator>{}
            );
        }

        partial_resolutions.at(SchemaTree::cRootId)
                .emplace_back(OUTCOME_TRYX(
                        QueryHandlerImpl::ColumnDescriptorTokenIterator::create(col.get())
                ));
    }

    std::vector<Expression*> ast_dfs_stack;
    ast_dfs_stack.emplace_back(query.get());
    while (false == ast_dfs_stack.empty()) {
        auto* expr{ast_dfs_stack.back()};
        ast_dfs_stack.pop_back();
        if (expr->has_only_expression_operands()) {
            for (auto it{expr->op_begin()}; it != expr->op_end(); ++it) {
                ast_dfs_stack.emplace_back(static_cast<Expression*>(it->get()));
            }
            continue;
        }

        auto* filter{dynamic_cast<FilterExpr*>(expr)};
        if (nullptr == filter) {
            continue;
        }

        auto* col{filter->get_column().get()};
        if (col->is_pure_wildcard() || col->get_descriptor_list().empty()) {
            continue;
        }

        bool const is_auto_gen{OUTCOME_TRYX(is_auto_generated(col->get_namespace()))};
        auto& partial_resolutions{
                is_auto_gen ? auto_gen_namespace_partial_resolutions
                            : user_gen_namespace_partial_resolutions
        };

        if (false == partial_resolutions.contains(SchemaTree::cRootId)) {
            partial_resolutions.emplace(
                    SchemaTree::cRootId,
                    std::vector<QueryHandlerImpl::ColumnDescriptorTokenIterator>{}
            );
        }

        auto const begin_token_it{
                OUTCOME_TRYX(QueryHandlerImpl::ColumnDescriptorTokenIterator::create(col))
        };
        partial_resolutions.at(SchemaTree::cRootId).emplace_back(begin_token_it);
        if (begin_token_it.is_wildcard()) {
            // To handle the case where the prefix wildcard matches nothing
            partial_resolutions.at(SchemaTree::cRootId)
                    .emplace_back(OUTCOME_TRYX(begin_token_it.next()));
        }
    }

    return {std::move(auto_gen_namespace_partial_resolutions),
            std::move(user_gen_namespace_partial_resolutions)};
}

auto is_auto_generated(std::string_view key_namespace) -> outcome_v2::std_result<bool> {
    if (clp_s::constants::cAutogenNamespace == key_namespace) {
        return true;
    }
    if (clp_s::constants::cDefaultNamespace == key_namespace) {
        return false;
    }
    return ErrorCode{ErrorCodeEnum::UnsupportedNamespace};
}
}  // namespace

auto QueryHandlerImpl::create(
        std::shared_ptr<Expression> query,
        std::vector<std::pair<std::string, LiteralTypeBitmask>> const& projections,
        bool case_sensitive_match
) -> outcome_v2::std_result<QueryHandlerImpl> {
    query = OUTCOME_TRYX(preprocess_query(query));
    auto projected_column_to_original_key{OUTCOME_TRYX(create_projection_map(projections))};
    auto [auto_gen_namespace_partial_resolutions, user_gen_namespace_partial_resolutions]{
            OUTCOME_TRYX(create_initial_partial_resolutions(query, projected_column_to_original_key)
            )
    };

    return QueryHandlerImpl{
            std::move(query),
            std::move(auto_gen_namespace_partial_resolutions),
            std::move(user_gen_namespace_partial_resolutions),
            std::move(projected_column_to_original_key),
            case_sensitive_match
    };
}

auto QueryHandlerImpl::evaluate_node_id_value_pairs(
        [[maybe_unused]] KeyValuePairLogEvent::NodeIdValuePairs const& auto_gen_node_id_value_pairs,
        [[maybe_unused]] KeyValuePairLogEvent::NodeIdValuePairs const& user_gen_node_id_value_pairs
) -> outcome_v2::std_result<AstEvaluationResult> {
    return ErrorCode{ErrorCodeEnum::MethodNotImplemented};
}
}  // namespace clp::ffi::ir_stream::search
