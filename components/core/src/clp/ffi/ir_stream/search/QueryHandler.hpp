#ifndef CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLER_HPP
#define CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLER_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include "../../../../clp_s/search/ast/Expression.hpp"
#include "../../../../clp_s/search/ast/Literal.hpp"
#include "../../KeyValuePairLogEvent.hpp"
#include "../../SchemaTree.hpp"
#include "AstEvaluationResult.hpp"
#include "NewProjectedSchemaTreeNodeCallbackReq.hpp"
#include "QueryHandlerImpl.hpp"

namespace clp::ffi::ir_stream::search {
/**
 * Handler for KV-pair IR stream search queries.
 *
 * Each query handler stores a KQL query represented as an AST. The handler is responsible for:
 * - resolving column descriptors for schema-tree nodes within the stream.
 * - evaluating the query against deserialized node-ID-value pairs.
 *
 * @tparam NewProjectedSchemaTreeNodeCallbackType Type of the callback to handle new projected
 * schema-tree nodes.
 */
template <NewProjectedSchemaTreeNodeCallbackReq NewProjectedSchemaTreeNodeCallbackType>
class QueryHandler {
public:
    // Factory function
    /**
     * @param new_projected_schema_tree_node_callback Callback for newly resolved projections.
     * @param query The search query.
     * @param projections The columns to project.
     * @param case_sensitive_match Whether to use case-sensitive match for string comparison.
     * @param allow_duplicate_projected_columns Whether to allow duplicate projected columns.
     * @return A result containing the newly constructed `QueryHandler` on success, or an error code
     * indicating the failure:
     * - Forwards `QueryHandlerImpl::create`'s return values.
     */
    [[nodiscard]] static auto create(
            NewProjectedSchemaTreeNodeCallbackType new_projected_schema_tree_node_callback,
            std::shared_ptr<clp_s::search::ast::Expression> query,
            std::vector<std::pair<std::string, clp_s::search::ast::literal_type_bitmask_t>> const&
                    projections,
            bool case_sensitive_match,
            bool allow_duplicate_projected_columns = false
    ) -> ystdlib::error_handling::Result<QueryHandler> {
        return QueryHandler{
                new_projected_schema_tree_node_callback,
                YSTDLIB_ERROR_HANDLING_TRYX(
                        QueryHandlerImpl::create(
                                std::move(query),
                                projections,
                                case_sensitive_match,
                                allow_duplicate_projected_columns
                        )
                )
        };
    }

    // Delete copy constructor and assignment operator
    QueryHandler(QueryHandler const&) = delete;
    auto operator=(QueryHandler const&) -> QueryHandler& = delete;

    // Default move constructor and assignment operator
    QueryHandler(QueryHandler&&) = default;
    auto operator=(QueryHandler&&) -> QueryHandler& = default;

    // Destructor
    ~QueryHandler() = default;

    /**
     * Updates any partially-resolved column descriptors by processing a newly inserted schema-tree
     * node.
     * @param is_auto_generated
     * @param node_locator
     * @param node_id
     * @return A void result on success, or an error code indicating the failure:
     * - Forwards `QueryHandlerImpl::update_partially_resolved_columns`'s return values.
     */
    [[nodiscard]] auto update_partially_resolved_columns(
            bool is_auto_generated,
            SchemaTree::NodeLocator const& node_locator,
            SchemaTree::Node::id_t node_id
    ) -> ystdlib::error_handling::Result<void> {
        return m_query_handler_impl.update_partially_resolved_columns(
                is_auto_generated,
                node_locator,
                node_id,
                m_new_projected_schema_tree_node_callback
        );
    }

    /**
     * Evaluates the given kv-pair log event against the underlying query.
     * @param log_event
     * @return A result containing the evaluation result on success, or an error code indicating
     * the failure:
     * - Forwards `QueryHandlerImpl::evaluate_kv_pair_log_event`'s return values.
     */
    [[nodiscard]] auto evaluate_kv_pair_log_event(KeyValuePairLogEvent const& log_event)
            -> ystdlib::error_handling::Result<AstEvaluationResult> {
        return m_query_handler_impl.evaluate_kv_pair_log_event(log_event);
    }

private:
    // Constructor
    explicit QueryHandler(
            NewProjectedSchemaTreeNodeCallbackType new_projected_schema_tree_node_callback,
            QueryHandlerImpl query_handler_impl
    )
            : m_new_projected_schema_tree_node_callback{new_projected_schema_tree_node_callback},
              m_query_handler_impl{std::move(query_handler_impl)} {}

    NewProjectedSchemaTreeNodeCallbackType m_new_projected_schema_tree_node_callback;
    QueryHandlerImpl m_query_handler_impl;
};
}  // namespace clp::ffi::ir_stream::search

#endif  // CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLER_HPP
