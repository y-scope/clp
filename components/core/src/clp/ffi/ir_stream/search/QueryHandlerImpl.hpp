#ifndef CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERIMPL_HPP
#define CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERIMPL_HPP

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include "../../../../clp_s/search/ast/AndExpr.hpp"
#include "../../../../clp_s/search/ast/ColumnDescriptor.hpp"
#include "../../../../clp_s/search/ast/EmptyExpr.hpp"
#include "../../../../clp_s/search/ast/Expression.hpp"
#include "../../../../clp_s/search/ast/FilterExpr.hpp"
#include "../../../../clp_s/search/ast/Literal.hpp"
#include "../../../../clp_s/search/ast/OrExpr.hpp"
#include "../../../../clp_s/search/ast/Value.hpp"
#include "../../KeyValuePairLogEvent.hpp"
#include "../../SchemaTree.hpp"
#include "AstEvaluationResult.hpp"
#include "ErrorCode.hpp"
#include "NewProjectedSchemaTreeNodeCallbackReq.hpp"
#include "utils.hpp"

namespace clp::ffi::ir_stream::search {
/**
 * Implementation of `QueryHandler`.
 */
class QueryHandlerImpl {
public:
    // Types
    /**
     * Iterator of column descriptor tokens.
     */
    class ColumnDescriptorTokenIterator {
    public:
        // Factory function
        /**
         * Creates an iterator that points to the first token in the given column descriptor.
         * @param column_descriptor
         * @return A result containing the constructed token iterator on success, or an error code
         * indicating the failure:
         * - ErrorCodeEnum::ColumnDescriptorTokenIteratorOutOfBounds if the column doesn't have any
         *   tokens.
         */
        [[nodiscard]] static auto create(clp_s::search::ast::ColumnDescriptor* column_descriptor)
                -> ystdlib::error_handling::Result<ColumnDescriptorTokenIterator> {
            auto const token_begin_it{column_descriptor->descriptor_begin()};
            if (column_descriptor->descriptor_end() == token_begin_it) {
                return ErrorCode{ErrorCodeEnum::ColumnDescriptorTokenIteratorOutOfBounds};
            }
            return ColumnDescriptorTokenIterator{column_descriptor, token_begin_it};
        }

        // Methods
        /**
         * @return Whether this is the last token of the underlying column descriptor.
         */
        [[nodiscard]] auto is_last() const -> bool {
            return m_column_descriptor->descriptor_end() == m_next_token_it;
        }

        /**
         * @return A result containing a newly constructor iterator pointing to the next token on
         * success, or an error code indicating the failure:
         * - ErrorCodeEnum::ColumnDescriptorTokenIteratorOutOfBounds if the current token is already
         *   the last one.
         */
        [[nodiscard]] auto next() const
                -> ystdlib::error_handling::Result<ColumnDescriptorTokenIterator> {
            if (is_last()) {
                return ErrorCode{ErrorCodeEnum::ColumnDescriptorTokenIteratorOutOfBounds};
            }
            return ColumnDescriptorTokenIterator{m_column_descriptor, m_next_token_it};
        }

        [[nodiscard]] auto is_wildcard() const -> bool { return m_curr_token_it->wildcard(); }

        [[nodiscard]] auto is_trailing_wildcard() const -> bool {
            return is_last() && is_wildcard();
        }

        [[nodiscard]] auto get_token() const -> std::string_view {
            return m_curr_token_it->get_token();
        }

        [[nodiscard]] auto get_column_descriptor() const -> clp_s::search::ast::ColumnDescriptor* {
            return m_column_descriptor;
        }

        /**
         * @param type
         * @return Whether the underlying column can match the given schema-tree node type.
         */
        [[nodiscard]] auto match_schema_tree_node_type(SchemaTree::Node::Type type) const -> bool {
            return m_column_descriptor->matches_any(schema_tree_node_type_to_literal_types(type));
        }

    private:
        // Constructor
        ColumnDescriptorTokenIterator(
                clp_s::search::ast::ColumnDescriptor* column_descriptor,
                clp_s::search::ast::DescriptorList::iterator curr_token_it
        )
                : m_column_descriptor{column_descriptor},
                  m_curr_token_it{curr_token_it},
                  m_next_token_it{curr_token_it + 1} {}

        clp_s::search::ast::ColumnDescriptor* m_column_descriptor;
        clp_s::search::ast::DescriptorList::iterator m_curr_token_it;
        clp_s::search::ast::DescriptorList::iterator m_next_token_it;
    };

    using ProjectionMap = std::unordered_map<clp_s::search::ast::ColumnDescriptor*, std::string>;

    using PartialResolutionMap = std::
            unordered_map<SchemaTree::Node::id_t, std::vector<ColumnDescriptorTokenIterator>>;

    // Factory function
    /**
     * @param query The search query.
     * @param projections The columns to project.
     * @param case_sensitive_match Whether to use case-sensitive match for string comparison.
     * @return A result containing the newly constructed `QueryHandler` on success, or an error code
     * indicating the failure:
     * - Forwards `preprocess_query`'s return values.
     * - Forwards `create_projected_columns_and_projection_map`'s return values.
     * - Forwards `create_initial_partial_resolutions`'s return values.
     */
    [[nodiscard]] static auto create(
            std::shared_ptr<clp_s::search::ast::Expression> query,
            std::vector<std::pair<std::string, clp_s::search::ast::literal_type_bitmask_t>> const&
                    projections,
            bool case_sensitive_match
    ) -> ystdlib::error_handling::Result<QueryHandlerImpl>;

    // Delete copy constructor and assignment operator
    QueryHandlerImpl(QueryHandlerImpl const&) = delete;
    auto operator=(QueryHandlerImpl const&) -> QueryHandlerImpl& = delete;

    // Default move constructor and assignment operator
    QueryHandlerImpl(QueryHandlerImpl&&) = default;
    auto operator=(QueryHandlerImpl&&) -> QueryHandlerImpl& = default;

    // Destructor
    ~QueryHandlerImpl() = default;

    /**
     * Implementation of `QueryHandler::evaluate_kv_pair_log_event`.
     * @param log_event
     * @return A result containing the evaluation result on success, or an error code indicating
     * the failure:
     * - ErrorCodeEnum::AstEvaluationInvariantViolation if the underlying AST DFS evaluation doesn't
     *   return any evaluation results.
     * - Forwards `AstExprIterator::create`'s return values.
     * - Forwards `advance_ast_dfs_evaluation`'s return values.
     */
    [[nodiscard]] auto evaluate_kv_pair_log_event(KeyValuePairLogEvent const& log_event)
            -> ystdlib::error_handling::Result<AstEvaluationResult>;

    /**
     * Implementation of `QueryHandler::update_partially_resolved_columns` with new projected
     * schema-tree node callback given as a template parameter.
     * @tparam NewProjectedSchemaTreeNodeCallbackType
     * @param is_auto_generated
     * @param node_locator
     * @param node_id
     * @param new_projected_schema_tree_node_callback
     * @return A result containing the evaluation result on success, or an error code indicating
     * the failure:
     * - Forwards `ColumnDescriptorTokenIterator::next`'s return values.
     * - Forwards `handle_column_resolution_on_new_schema_tree_node`'s return values.
     */
    template <NewProjectedSchemaTreeNodeCallbackReq NewProjectedSchemaTreeNodeCallbackType>
    [[nodiscard]] auto update_partially_resolved_columns(
            bool is_auto_generated,
            SchemaTree::NodeLocator const& node_locator,
            SchemaTree::Node::id_t node_id,
            NewProjectedSchemaTreeNodeCallbackType new_projected_schema_tree_node_callback
    ) -> ystdlib::error_handling::Result<void>;

    [[nodiscard]] auto get_resolved_column_to_schema_tree_node_ids() const -> std::unordered_map<
            clp_s::search::ast::ColumnDescriptor*,
            std::unordered_set<SchemaTree::Node::id_t>> {
        return m_resolved_column_to_schema_tree_node_ids;
    }

private:
    // Types
    /**
     * Iterator for efficiently traversing and evaluating clp-s AST's expressions.
     */
    class AstExprIterator {
    public:
        // Factory function
        /**
         * @param expr
         * @return A result containing the constructed iterator for `expr`, or an error code
         * indicating the failure:
         * - ErrorCodeEnum::ExpressionTypeUnexpected if the type of expression is not one of the
         *   following:
         *   - clp_s::search::ast::FilterExpr
         *   - clp_s::search::ast::AndExpr
         *   - clp_s::search::ast::OrExpr
         */
        [[nodiscard]] static auto create(clp_s::search::ast::Value* expr)
                -> ystdlib::error_handling::Result<AstExprIterator>;

        // Methods
        /**
         * Retrieves the next child expression operator to visit as an `AstExprIterator`.
         * @return A result containing the next `AstExprIterator` to visit on success, or an error
         * code indicating the failure:
         * - ErrorCodeEnum::AttemptToIterateAstLeafExpr if the current expression is a leaf
         *   expression (`clp_s::search::ast::FilterExpr`) with no child operators.
         * - Forwards `create`'s return values.
         * @return std::nullopt if there are no more child operators to visit.
         */
        [[nodiscard]] auto next_op()
                -> std::optional<ystdlib::error_handling::Result<AstExprIterator>>;

        /**
         * @return The underlying expression as `clp_s::search::ast::AndExpr` if the underlying type
         * matches, or nullptr otherwise.
         */
        [[nodiscard]] auto as_and_expr() const -> clp_s::search::ast::AndExpr const* {
            if (std::holds_alternative<clp_s::search::ast::AndExpr*>(m_expr)) {
                return std::get<clp_s::search::ast::AndExpr*>(m_expr);
            }
            return nullptr;
        }

        /**
         * @return The underlying expression as `clp_s::search::ast::OrExpr` if the underlying type
         * matches, or nullptr otherwise.
         */
        [[nodiscard]] auto as_or_expr() const -> clp_s::search::ast::OrExpr const* {
            if (std::holds_alternative<clp_s::search::ast::OrExpr*>(m_expr)) {
                return std::get<clp_s::search::ast::OrExpr*>(m_expr);
            }
            return nullptr;
        }

        /**
         * @return The underlying expression as `clp_s::search::ast::FilterExpr` if the underlying
         * type matches, or nullptr otherwise.
         */
        [[nodiscard]] auto as_filter_expr() const -> clp_s::search::ast::FilterExpr* {
            if (std::holds_alternative<clp_s::search::ast::FilterExpr*>(m_expr)) {
                return std::get<clp_s::search::ast::FilterExpr*>(m_expr);
            }
            return nullptr;
        }

        [[nodiscard]] auto is_inverted() const -> bool { return m_is_inverted; }

    private:
        // Types
        using ExprVariant = std::variant<
                clp_s::search::ast::AndExpr*,
                clp_s::search::ast::OrExpr*,
                clp_s::search::ast::FilterExpr*>;

        // Constructor
        AstExprIterator(
                ExprVariant expr,
                clp_s::search::ast::OpList::const_iterator op_next_it,
                clp_s::search::ast::OpList::const_iterator op_end_it,
                bool is_inverted
        )
                : m_expr{expr},
                  m_op_next_it{op_next_it},
                  m_op_end_it{op_end_it},
                  m_is_inverted{is_inverted} {}

        // Variables
        ExprVariant m_expr;
        clp_s::search::ast::OpList::const_iterator m_op_next_it;
        clp_s::search::ast::OpList::const_iterator m_op_end_it;
        bool m_is_inverted;
    };

    // Constructor
    QueryHandlerImpl(
            std::shared_ptr<clp_s::search::ast::Expression> query,
            PartialResolutionMap auto_gen_namespace_partial_resolutions,
            PartialResolutionMap user_gen_namespace_partial_resolutions,
            std::vector<std::shared_ptr<clp_s::search::ast::ColumnDescriptor>> projected_columns,
            ProjectionMap projected_column_to_original_key,
            bool case_sensitive_match
    )
            : m_query{std::move(query)},
              m_is_empty_query{
                      nullptr != dynamic_cast<clp_s::search::ast::EmptyExpr*>(m_query.get())
              },
              m_auto_gen_namespace_partial_resolutions{
                      std::move(auto_gen_namespace_partial_resolutions)
              },
              m_user_gen_namespace_partial_resolutions{
                      std::move(user_gen_namespace_partial_resolutions)
              },
              m_projected_columns{std::move(projected_columns)},
              m_projected_column_to_original_key{std::move(projected_column_to_original_key)},
              m_case_sensitive_match{case_sensitive_match} {}

    // Methods
    /**
     * Handles column resolution of the given token iterator on the a new schema-tree tree node.
     * @tparam NewProjectedSchemaTreeNodeCallbackType
     * @param is_auto_generated
     * @param node_id
     * @param node_locator
     * @param token_it The current resolved token in the column.
     * @param new_projected_schema_tree_node_callback
     * @return A void result on success, or an error code indicating the failure:
     * - Forwards `ColumnDescriptorTokenIterator::next`'s return values.
     * - Forwards `new_projected_schema_tree_node_callback`'s return values.
     */
    template <NewProjectedSchemaTreeNodeCallbackReq NewProjectedSchemaTreeNodeCallbackType>
    [[nodiscard]] auto handle_column_resolution_on_new_schema_tree_node(
            bool is_auto_generated,
            SchemaTree::Node::id_t node_id,
            SchemaTree::NodeLocator const& node_locator,
            ColumnDescriptorTokenIterator const& token_it,
            NewProjectedSchemaTreeNodeCallbackType new_projected_schema_tree_node_callback
    ) -> ystdlib::error_handling::Result<void>;

    /**
     * Evaluates the filter expression against the given kv-pair log event.
     * @param filter_expr
     * @param log_event
     * @return A result containing the evaluation result on success, or an error code indicating the
     * failure:
     * - ErrorCodeEnum::AstEvaluationInvariantViolation if the underlying column of the filter has
     *   been resolved, but is neither user-generated nor auto-generated.
     * - Forwards `evaluate_wildcard_filter`'s return values.
     * - Forwards `evaluate_filter_against_node_id_value_pair`'s return values.
     */
    [[nodiscard]] auto evaluate_filter_expr(
            clp_s::search::ast::FilterExpr* filter_expr,
            KeyValuePairLogEvent const& log_event
    ) -> ystdlib::error_handling::Result<AstEvaluationResult>;

    auto push_to_ast_dfs_stack(AstExprIterator ast_expr_it) -> void {
        m_ast_dfs_stack.emplace_back(ast_expr_it, ast_evaluation_result_bitmask_t{});
    }

    /**
     * Pops from the AST DFS stack and updates the evaluation result accordingly:
     * - If the stack still has elements, updates the parent's evaluation results.
     * - Otherwise, updates `query_evaluation_result`.
     * @param evaluation_result
     * @param query_evaluation_result Returns the query evaluation result.
     */
    auto pop_from_ast_dfs_stack_and_update_evaluation_results(
            AstEvaluationResult evaluation_result,
            std::optional<AstEvaluationResult>& query_evaluation_result
    ) -> void;

    /**
     * Advances the AST DFS evaluation by visiting the top of `m_ast_dfs_stack`.
     * @param log_event
     * @param query_evaluation_result Returns the query evaluation result.
     * @return A void result on success, or an error code indicating the failure:
     * - ErrorCodeEnum::AstEvaluationInvariantViolation if the expression iterator at the top of the
     *   stack is an unexpected expression type.
     * - Forwards `evaluate_filter_expr`'s return values.
     * - Forwards `AstExprIterator::next_op`'s return values.
     */
    [[nodiscard]] auto advance_ast_dfs_evaluation(
            KeyValuePairLogEvent const& log_event,
            std::optional<AstEvaluationResult>& query_evaluation_result
    ) -> ystdlib::error_handling::Result<void>;

    // Variables
    std::shared_ptr<clp_s::search::ast::Expression> m_query;
    bool m_is_empty_query;
    PartialResolutionMap m_auto_gen_namespace_partial_resolutions;
    PartialResolutionMap m_user_gen_namespace_partial_resolutions;
    std::unordered_map<
            clp_s::search::ast::ColumnDescriptor*,
            std::unordered_set<SchemaTree::Node::id_t>>
            m_resolved_column_to_schema_tree_node_ids;
    std::vector<std::shared_ptr<clp_s::search::ast::ColumnDescriptor>> m_projected_columns;
    ProjectionMap m_projected_column_to_original_key;
    bool m_case_sensitive_match;
    std::vector<std::pair<AstExprIterator, ast_evaluation_result_bitmask_t>> m_ast_dfs_stack;
};

template <NewProjectedSchemaTreeNodeCallbackReq NewProjectedSchemaTreeNodeCallbackType>
auto QueryHandlerImpl::update_partially_resolved_columns(
        bool is_auto_generated,
        SchemaTree::NodeLocator const& node_locator,
        SchemaTree::Node::id_t node_id,
        NewProjectedSchemaTreeNodeCallbackType new_projected_schema_tree_node_callback
) -> ystdlib::error_handling::Result<void> {
    auto const parent_node_id{node_locator.get_parent_id()};
    auto& partial_resolutions_to_update{
            is_auto_generated ? m_auto_gen_namespace_partial_resolutions
                              : m_user_gen_namespace_partial_resolutions
    };
    if (false == partial_resolutions_to_update.contains(parent_node_id)) {
        return ystdlib::error_handling::success();
    }

    std::vector<std::pair<SchemaTree::Node::id_t, ColumnDescriptorTokenIterator>>
            new_partial_resolutions;
    for (auto const& token_it : partial_resolutions_to_update.at(parent_node_id)) {
        YSTDLIB_ERROR_HANDLING_TRYV(handle_column_resolution_on_new_schema_tree_node(
                is_auto_generated,
                node_id,
                node_locator,
                token_it,
                new_projected_schema_tree_node_callback
        ));

        // Handle partial resolution updates
        if (SchemaTree::Node::Type::Obj != node_locator.get_type()) {
            // If the schema-tree node is not of type `Obj`, it cannot have children, so no further
            // resolution is possible from this node.
            continue;
        }

        if (token_it.is_wildcard()) {
            // For wildcard tokens, simulate potential matches by:
            // - Adding a resolution that assumes this node matches part of the wildcard.
            // - If this isn't the last wildcard token, also add a resolution for continuing the
            //   match.
            new_partial_resolutions.emplace_back(node_id, token_it);
            if (false == token_it.is_last()) {
                new_partial_resolutions.emplace_back(
                        node_id,
                        YSTDLIB_ERROR_HANDLING_TRYX(token_it.next())
                );
            }
            continue;
        }

        if (token_it.is_last() || token_it.get_token() != node_locator.get_key_name()) {
            // If this is the last token, or the token doesn't match the node's key, then no further
            // resolution is possible through the given node.
            continue;
        }

        auto const next_token_it{YSTDLIB_ERROR_HANDLING_TRYX(token_it.next())};
        new_partial_resolutions.emplace_back(node_id, next_token_it);
        if (false == next_token_it.is_last() && next_token_it.is_wildcard()) {
            // Handle the case where the wildcard matches nothing
            new_partial_resolutions.emplace_back(
                    node_id,
                    YSTDLIB_ERROR_HANDLING_TRYX(next_token_it.next())
            );
        }
    }

    for (auto const [node_id, token_it] : new_partial_resolutions) {
        auto [it, inserted] = partial_resolutions_to_update.try_emplace(
                node_id,
                std::vector<ColumnDescriptorTokenIterator>{}
        );
        it->second.emplace_back(token_it);
    }

    return ystdlib::error_handling::success();
}

template <NewProjectedSchemaTreeNodeCallbackReq NewProjectedSchemaTreeNodeCallbackType>
auto QueryHandlerImpl::handle_column_resolution_on_new_schema_tree_node(
        bool is_auto_generated,
        SchemaTree::Node::id_t node_id,
        SchemaTree::NodeLocator const& node_locator,
        QueryHandlerImpl::ColumnDescriptorTokenIterator const& token_it,
        NewProjectedSchemaTreeNodeCallbackType new_projected_schema_tree_node_callback
) -> ystdlib::error_handling::Result<void> {
    if ((false == token_it.is_last()
         && false == YSTDLIB_ERROR_HANDLING_TRYX(token_it.next()).is_trailing_wildcard())
        || false == token_it.match_schema_tree_node_type(node_locator.get_type())
        || (false == token_it.is_wildcard() && token_it.get_token() != node_locator.get_key_name()))
    {
        // If the given token:
        // - is neither the end token nor the last token before a trailing wildcard
        // - doesn't match the new node's type
        // - is neither a wildcard nor equal to the new node's key
        // There should be no resolution.
        return ystdlib::error_handling::success();
    }

    auto* col{token_it.get_column_descriptor()};
    if (m_projected_column_to_original_key.contains(col)) {
        YSTDLIB_ERROR_HANDLING_TRYV(new_projected_schema_tree_node_callback(
                is_auto_generated,
                node_id,
                m_projected_column_to_original_key.at(col)
        ));
        return ystdlib::error_handling::success();
    }

    auto [it, inserted] = m_resolved_column_to_schema_tree_node_ids.try_emplace(
            col,
            std::unordered_set<SchemaTree::Node::id_t>{}
    );
    it->second.emplace(node_id);
    return ystdlib::error_handling::success();
}
}  // namespace clp::ffi::ir_stream::search

#endif  // CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERIMPL_HPP
